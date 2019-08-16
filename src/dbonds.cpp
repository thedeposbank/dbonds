#include <dbonds.hpp>
#include <utility.hpp>

#include <string>
#include <eosio/system.hpp>

void dbonds::check_on_transfer(name from, name to, asset quantity, const string& memo) {
  check(from != to, "cannot transfer to self");
  require_auth(from);
  check(is_account(to), "to account does not exist");
  auto sym = quantity.symbol.code();
  
  stats statstable(_self, _self.value);
  const auto& st = statstable.get(sym.raw(), "no stats for given symbol code");

  require_recipient(from);
  require_recipient(to);
  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount == 1 && quantity.symbol.precision() == 0, "quantity must be 1");
  check(memo.size() <= 256, "memo has more than 256 bytes");
}

void dbonds::check_on_fcdb_transfer(name from, name to, asset quantity, const string & memo){
  
  check_on_transfer(from, to, quantity, memo);
  // find dbond in cusom table with all info

  auto sym = quantity.symbol.code();
  stats statstable(_self, sym.raw());
  const auto& st = statstable.get(sym.raw(), "no stats for given symbol code");
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.get(quantity.symbol.raw(), "FATAL ERROR: dbond not found in fc_dbond table");

  bool to_in_holders = false;
  for(auto acc : fcdb_info.dbond.holders_list){
    if(to == acc){
      to_in_holders = true;
      break;
    }
  }
  check(to_in_holders, "error, trying to send dbond to the one, who is not in the holders_list");
}

void dbonds::set_initial_data(dbond_id_class dbond_id) {
  stats statstable(_self, _self.value);
  const auto& st = statstable.get(dbond_id.raw(), "no stats for given symbol code");
  
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fc_dbond table");

  fcdb_stat.modify(fcdb_info, _self, [&](auto& s) {
    s.initial_price = s.current_price;
    s.initial_time  = current_time_point();
  });
}

ACTION dbonds::transfer(name from, name to, asset quantity, const string& memo) {
  
  check_on_fcdb_transfer(from, to, quantity, memo);
  
  auto payer = has_auth(to) ? to : from;

  sub_balance(from, quantity);
  add_balance(to, quantity, payer);
}

ACTION dbonds::create(name issuer, asset maximum_supply) {

  // check(has_auth(_self) || has_auth(DBVERIFIER), "auth required");
  require_auth(_self);

  auto sym = maximum_supply.symbol;
  check(sym.is_valid(), "invalid dbond name");
  check(maximum_supply.is_valid(), "invalid supply");
  check(maximum_supply.amount > 0, "max-supply must be positive");

  stats statstable(_self, sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing == statstable.end(), "dbond with id already exists");

  statstable.emplace(_self, [&](auto& s) {
    s.supply.symbol = maximum_supply.symbol;
    s.max_supply    = maximum_supply;
    s.issuer        = issuer;
  });
}

ACTION dbonds::issue(name to, asset quantity, string memo){
  auto sym = quantity.symbol;
  check( sym.is_valid(), "invalid symbol name" );
  check( memo.size() <= 256, "memo has more than 256 bytes" );

  stats statstable( _self, sym.code().raw() );
  auto existing = statstable.find( sym.code().raw() );
  check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
  const auto& st = *existing;

  require_auth(st.issuer);

  check( quantity.is_valid(), "invalid quantity" );
  check( quantity.amount > 0, "must issue positive quantity" );

  check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
  check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

  statstable.modify( st, same_payer, [&]( auto& s ) {
    s.supply += quantity;
  });

  add_balance( st.issuer, quantity, st.issuer );

  if( to != st.issuer ) {
    SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} }, { st.issuer, to, quantity, memo } );
  }
}

ACTION dbonds::burn(name from, dbond_id_class dbond_id) {}

ACTION dbonds::initfcdb(fc_dbond & bond) {

  require_auth(bond.emitent);

  check_fc_dbond_sanity(bond);

  // find dbond in common table
  stats statstable(_self, bond.bond_name.raw());
  auto dbond_stat = statstable.find(bond.bond_name.raw());

  // if not exists, call create ACTION
  if(dbond_stat == statstable.end()){
    SEND_INLINE_ACTION(*this, create, {{_self, "active"_n}}, {bond.emitent, bond.quantity_to_issue});
  }

  // find dbond in cusom table with all info
  fc_dbond_index fcdb_stat(_self, bond.emitent.value);
  auto fcdb_info = fcdb_stat.find(bond.bond_name.raw());

  if(fcdb_info == fcdb_stat.end()) {
    // new dbond, make a record for it
    fcdb_stat.emplace(bond.emitent, [&](auto& s) {
      s.dbond        = bond;
      s.initial_time = time_point();
      s.fc_state     = (int)utility::fc_dbond_state::CREATED;
    });
  } 
  else if(fcdb_info->fc_state == (int)utility::fc_dbond_state::CREATED) {
    // dbond already exists, but in state CREATED it may be overwritten
    fcdb_stat.modify(fcdb_info, bond.emitent, [&](auto& s) {
      s.dbond      = bond;
    });
  } 
  else {
    check(false, "dbond exists and not in CREATED state, change is not allowed");
  }
}

ACTION dbonds::verifyfcdb(name from, dbond_id_class dbond_id) {
  
  require_auth(from);

  // find dbond in common table
  stats statstable(_self, dbond_id.raw());
  auto st = statstable.get(dbond_id.raw(), "dbond not found");

  // find dbond in custom table with al info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fc_dbond table");

  // check that from == dbond.verifier
  check(fcdb_info.dbond.verifier == from, "you must be verifier for this dbond to call this ACTION");

  change_fcdb_state(dbond_id, (int)utility::fc_dbond_state::AGREEMENT_SIGNED);
}

ACTION dbonds::issuefcdb(name from, dbond_id_class dbond_id) {
  require_auth(from);

  // check bond exists
  stats statstable(_self, dbond_id.raw());
  auto st = statstable.get(dbond_id.raw(), "dbond not found");

  // get dbond info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fcdbond table");

  // check from == emitent
  check(fcdb_info.dbond.emitent == from, "you must be an dbond.eminent to call this ACTION");

  // check dbond is in state AGREEMENT_SIGNED
  check(fcdb_info.fc_state == (int)utility::fc_dbond_state::AGREEMENT_SIGNED, "wrong fc_dbond state to call this ACTION");

  // call classic action issue
  SEND_INLINE_ACTION(*this, issue, {{_self, "active"_n}}, {fcdb_info.dbond.emitent, fcdb_info.dbond.quantity_to_issue, ""});

  // change state of dbond according to logic
  change_fcdb_state(dbond_id, (int)utility::fc_dbond_state::CIRCULATING);

  // update dbond price
  SEND_INLINE_ACTION(*this, updfcdbprice, {{_self, "active"_n}}, {dbond_id});

  // set initial time and price
  set_initial_data(dbond_id);
}

ACTION dbonds::updfcdbprice(dbond_id_class dbond_id) {
}

ACTION dbonds::confirmfcdb(dbond_id_class dbond_id) {
  stats statstable(_self, dbond_id.raw());
  const auto st = statstable.get(dbond_id.raw(), "dbond not found");

  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.find(dbond_id.raw());
  check(fcdb_info != fcdb_stat.end(), "FATAL ERROR: dbond not found in fc_dbond table");

  // can be called only by dbond.counterparty
  require_auth(fcdb_info->dbond.counterparty);

  // check that is not confirmed yet
  check(fcdb_info->confirmed_by_counterparty != 1, "dbond is already confirmed by counterparty");

  // change make confirmation
  fcdb_stat.modify(fcdb_info, same_payer, [&](auto& stat) {
    stat.confirmed_by_counterparty = 1;
  });
}

#ifdef DEBUG
ACTION dbonds::erase(name owner, dbond_id_class dbond_id) {
  stats statstable(_self, dbond_id.raw());
  const auto st = statstable.find(dbond_id.raw());
  if(st != statstable.end()) {
    name emitent = st->issuer;
    // stat:
    statstable.erase(st);

    // for fc_dbond:
    fc_dbond_index fcdb_stat(_self, emitent.value);
    auto fcdb_info = fcdb_stat.find(dbond_id.raw());
    if(fcdb_info != fcdb_stat.end()) {
      fcdb_stat.erase(fcdb_info);
    }
  }

  // balance:
  if(owner != ""_n) {
    accounts acnts(_self, owner.value);
    const auto account = acnts.find(dbond_id.raw());
    if(account != acnts.end()) {
      acnts.erase(account);
    }
  }
}
#endif

void dbonds::ontransfer(name from, name to, asset quantity, const string& memo) {}

//////////////////////////////////////////////////////////

void dbonds::sub_balance(name owner, asset value){
  accounts from_acnts(_self, owner.value);

  const auto& from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  #ifdef DEBUG
    name ram_payer = _self;
  #else
    name ram_payer = owner;
  #endif
  from_acnts.modify(from, ram_payer, [&](auto& a) {
    a.balance -= value;
  });
}

void dbonds::add_balance(name owner, asset value, name ram_payer){
  accounts to_acnts(_self, owner.value);
  auto to = to_acnts.find(value.symbol.code().raw());
  if(to == to_acnts.end()) {
    to_acnts.emplace(ram_payer, [&](auto& a){
      a.balance = value;
    });
  } else {
    to_acnts.modify(to, same_payer, [&](auto& a) {
      a.balance += value;
    });
  }
}

void dbonds::check_fc_dbond_sanity(const fc_dbond& bond) {

  check(is_account(bond.verifier), "verifier account does not exist");

  check(bond.holders_list.size() < utility::max_holders_number, "there cannot be that many holders of the dbond");

  bool dbonds_in_holders = false;
  bool emitent_in_holders = false;
  for (auto acc : bond.holders_list){
    check(is_account(acc), "one of dbond holders is not a valid account");
    if(acc == bond.emitent)
      emitent_in_holders = true;
    if(acc == _self)
      dbonds_in_holders = true;
  }
  check(emitent_in_holders, "you need to add your account to the holders_list");
  check(dbonds_in_holders, "you need to add thedbondsacc to the holders_list");

  check(bond.maturity_time >= current_time_point() + WEEK_uSECONDS, 
    "maturity_time is too close to the current time_point");

  check(bond.maturity_time + WEEK_uSECONDS >= bond.collateral_bond.maturity_time,
    "dbond maturity_time is too far from fiat bond maturity time");

  check(bond.maturity_time <= bond.collateral_bond.maturity_time,
    "dbond maturity_time must be not earlier than the fiat bond maturity time");
}

void dbonds::change_fcdb_state(dbond_id_class dbond_id, int new_state){
  check(new_state >= int(utility::fc_dbond_state::First)
    && new_state <= int(utility::fc_dbond_state::Last), "wrong state to change to");
  
  // check bond exists
  stats statstable(_self, dbond_id.raw());
  auto st = statstable.get(dbond_id.raw(), "dbond not found");

  // get dbond info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.find(dbond_id.raw());
    
  fcdb_stat.modify(fcdb_info, same_payer, [&](auto& stat) {
    stat.fc_state = new_state;
  });
}
