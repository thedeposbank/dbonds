#include <dbonds.hpp>
#include <utility.hpp>

#include <string>
#include <eosio/system.hpp>

void dbonds::check_on_transfer(name from, name to, asset quantity, const string& memo) {
  check(from != to, "cannot transfer to self");
  require_auth(from);
  check(is_account(to), "to account does not exist");
  auto sym = quantity.symbol.code();
  
  stats statstable(_self, sym.raw());
  const auto& st = statstable.get(sym.raw(), "no stats for given symbol code");

  require_recipient(from);
  require_recipient(to);
  check(quantity.is_valid(), "invalid quantity");
  check(memo.size() <= 256, "memo has more than 256 bytes");
}

void dbonds::check_on_fcdb_transfer(name from, name to, asset quantity, const string & memo){
  
  check_on_transfer(from, to, quantity, memo);
  // find dbond in cusom table with all info

  auto sym = quantity.symbol.code();
  stats statstable(_self, sym.raw());
  const auto& st = statstable.get(sym.raw(), "no stats for given symbol code");
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  const auto& fcdb_info = fcdb_stat.get(sym.raw(), "FATAL ERROR: dbond not found in fc_dbond table");

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
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw(), "no stats for given symbol code");
  
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  const auto& fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fc_dbond table");

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

  symbol_code dbond_id;
  if(to == _self && utility::match_memo(memo, "retire ", dbond_id)) {
    check(quantity.symbol.code() == dbond_id, "wrong dbond id");
    retire_fcdb(dbond_id, extended_asset{quantity, _self});
  }
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

ACTION dbonds::issue(name to, asset quantity, string memo) {
  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  stats statstable(_self, sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
  const auto& st = *existing;

  // allow only inline action calls
  require_auth(_self);

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must issue positive quantity");

  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

  statstable.modify(st, _self, [&](auto& s) {
    s.supply += quantity;
  });

  add_balance(st.issuer, quantity, _self);
  // print("\nline: ", __LINE__); check(false, "bye");

  if(to != st.issuer) {
    SEND_INLINE_ACTION(*this, transfer, {{st.issuer, "active"_n}}, {st.issuer, to, quantity, memo});
  }
}

ACTION dbonds::burn(name from, dbond_id_class dbond_id) {}

ACTION dbonds::initfcdb(fc_dbond & bond) {

  require_auth(bond.emitent);

  check_fcdb_sanity(bond);

  // find dbond in common table
  stats statstable(_self, bond.dbond_id.raw());
  auto dbond_stat = statstable.find(bond.dbond_id.raw());

  // if not exists, call create ACTION
  if(dbond_stat == statstable.end()){
    SEND_INLINE_ACTION(*this, create, {{_self, "active"_n}}, {bond.emitent, bond.quantity_to_issue});
  }

  // find dbond in cusom table with all info
  fc_dbond_index fcdb_stat(_self, bond.emitent.value);
  auto fcdb_info = fcdb_stat.find(bond.dbond_id.raw());

  if(fcdb_info == fcdb_stat.end()) {
    // new dbond, make a record for it
    fcdb_stat.emplace(bond.emitent, [&](auto& s) {
      s.dbond        = bond;
      s.initial_time = time_point();
      s.fc_state     = (int)utility::fcdb_state::CREATED;
    });
  } 
  else if(fcdb_info->fc_state == (int)utility::fcdb_state::CREATED) {
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
  const auto& st = statstable.get(dbond_id.raw(), "dbond not found");

  // find dbond in custom table with al info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  const auto& fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fc_dbond table");

  // check that from == dbond.verifier
  check(fcdb_info.dbond.verifier == from, "you must be verifier for this dbond to call this ACTION");

  change_fcdb_state(dbond_id, utility::fcdb_state::AGREEMENT_SIGNED);
}

ACTION dbonds::issuefcdb(name from, dbond_id_class dbond_id) {
  require_auth(from);

  // check bond exists
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw(), "dbond not found");

  // get dbond info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  const auto& fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fcdbond table");

  // check from == emitent
  check(fcdb_info.dbond.emitent == from, "you must be an dbond.eminent to call this ACTION");

  // check dbond is in state AGREEMENT_SIGNED
  check(fcdb_info.fc_state == (int)utility::fcdb_state::AGREEMENT_SIGNED, "wrong fc_dbond state to call this ACTION");

  // call classic action issue
  SEND_INLINE_ACTION(*this, issue, {{_self, "active"_n}}, {fcdb_info.dbond.emitent, fcdb_info.dbond.quantity_to_issue, std::string{}});

  // change state of dbond according to logic
  change_fcdb_state(dbond_id, utility::fcdb_state::CIRCULATING);

  // update dbond price
  SEND_INLINE_ACTION(*this, updfcdb, {{_self, "active"_n}}, {dbond_id});
}

ACTION dbonds::updfcdb(dbond_id_class dbond_id) {
  stats statstable(_self, dbond_id.raw());
  const auto st = statstable.get(dbond_id.raw(), "dbond not found");

  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.find(dbond_id.raw());
  check(fcdb_info != fcdb_stat.end(), "FATAL ERROR: dbond not found in fc_dbond table");

  // update price
  uint32_t maturity_time = fcdb_info->dbond.maturity_time.sec_since_epoch();
  uint32_t current_time = current_time_point().sec_since_epoch();
  int64_t s_to_maturity = (maturity_time - current_time);
  int64_t s_in_year = 365LL * 24 * 60 * 60;
  double cur_price = 0;
  if(s_to_maturity > 0){
    double b = 1.0 * fcdb_info->dbond.payoff_price.quantity.amount;
    double apr = 1.0 * fcdb_info->dbond.apr;
    cur_price = b / (1. + apr / 1e4 * s_to_maturity / s_in_year);
  }

  extended_asset new_price = extended_asset((int64_t)(cur_price+0.99), fcdb_info->dbond.payoff_price.get_extended_symbol());

  fcdb_stat.modify(fcdb_info, same_payer, [&](auto& a) {
      a.current_price = new_price;
  });

  if(fcdb_info->initial_price.quantity.amount == 0) {
    // set initial time and price
    set_initial_data(dbond_id);
  }

  // update state
  time_point now = current_time_point();
  if(now >= fcdb_info->dbond.retire_time) {
    if(fcdb_info->fc_state == (int)utility::fcdb_state::EXPIRED_TECH_DEFAULTED) {
      change_fcdb_state(dbond_id, utility::fcdb_state::EXPIRED_DEFAULTED);
    }
    return;
  }
  if(now >= fcdb_info->dbond.maturity_time &&
      fcdb_info->fc_state == (int)utility::fcdb_state::CIRCULATING) {
    if(get_balance(_self, fcdb_info->dbond.emitent, dbond_id) == st.supply) {
      change_fcdb_state(dbond_id, utility::fcdb_state::EXPIRED_PAID_OFF);
    }
    else {
      change_fcdb_state(dbond_id, utility::fcdb_state::EXPIRED_TECH_DEFAULTED);
    }
  }
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

  // check that dbond is verified and would not change
  check(fcdb_info->fc_state >= (int)utility::fcdb_state::AGREEMENT_SIGNED, "dbond is not verified");

  // change make confirmation
  fcdb_stat.modify(fcdb_info, same_payer, [&](auto& stat) {
    stat.confirmed_by_counterparty = 1;
  });
}

ACTION dbonds::delunissued(dbond_id_class dbond_id) {
  // check bond exists
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw(), "dbond not found");

  // get dbond info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  const auto& fcdb_info = fcdb_stat.get(dbond_id.raw(), "FATAL ERROR: dbond not found in fcdbond table");
  
  require_auth(fcdb_info.dbond.emitent);

  check(fcdb_info.fc_state < (int)utility::fcdb_state::CIRCULATING, "emitent can erase token only if it is not issued yet");

  erase_dbond(dbond_id);

}

#ifdef DEBUG
ACTION dbonds::erase(name owner, dbond_id_class dbond_id) {
  stats statstable(_self, dbond_id.raw());
  auto st = statstable.find(dbond_id.raw());
  if(st != statstable.end()) {
    name emitent = st->issuer;
    // stat:
    statstable.erase(st);

    fc_dbond_index fcdb_stat(_self, emitent.value);
    auto fcdb_info = fcdb_stat.find(dbond_id.raw());

    if(fcdb_info != fcdb_stat.end()) {
      // balances:
      for(auto holder : fcdb_info->dbond.holders_list) {
        accounts acnts(_self, holder.value);
        auto account = acnts.find(dbond_id.raw());
        if(account != acnts.end()) {
          acnts.erase(account);
        }
      }
      // for fc_dbond:
      fcdb_stat.erase(fcdb_info);
    }
  }
}

ACTION dbonds::setstate(dbond_id_class dbond_id, int state) {
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw());

  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  const auto& fcdb_info = fcdb_stat.get(dbond_id.raw());

  fcdb_stat.modify(fcdb_info, st.issuer, [&](auto& s) {
    s.fc_state = state;
  });
}
#endif

void dbonds::ontransfer(name from, name to, asset quantity, const string& memo) {
  if(to == _self) {
    name token_contract = get_first_receiver();
    dbond_id_class dbond_id;
    if(utility::match_memo(memo, "retire ", dbond_id)) {
      // fail tx, if dbond_id is empty
      check(dbond_id != symbol_code(), "undefined dbond id");

      retire_fcdb(dbond_id, extended_asset{quantity, token_contract});
      return;
    }
  }
}

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

void dbonds::check_fcdb_sanity(const fc_dbond& bond) {

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

  check(bond.collateral_bond.maturity_time + WEEK_uSECONDS <= bond.retire_time,
    "dbond retire_time must be at least a week later than bond maturity time");
}

void dbonds::erase_dbond(dbond_id_class dbond_id) {
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw(), "dbond not found");

  check(get_balance(_self, _self, dbond_id) == st.supply, "can erase only if all tokens are at dBonds contract");

  name emitent = st.issuer;
  // burn all dbond tokens and delete info from the table

  accounts dbonds_acnt(_self, _self.value);
  auto dbonds_ac = dbonds_acnt.find(dbond_id.raw());
  if(dbonds_ac != dbonds_acnt.end())
    dbonds_acnt.erase(dbonds_ac);

  fc_dbond_index fcdb(_self, emitent.value);
  fcdb.erase(fcdb.get(dbond_id.raw()));

  statstable.erase(st);
}

void dbonds::on_final_state(const fc_dbond_stats& fcdb_info) {
  dbond_id_class dbond_id = fcdb_info.dbond.dbond_id;
  // enforce explicit transfers from ALL holders to dBonds account
  for(const auto& holder : fcdb_info.dbond.holders_list) {
    accounts acnt(_self, holder.value);
    asset balance = get_balance(_self, holder, dbond_id);
    if(balance.amount != 0) {
      sub_balance(holder, balance);
      add_balance(_self, balance, _self);
    }
  }

  // erase_dbond(dbond_id);
}

void dbonds::change_fcdb_state(dbond_id_class dbond_id, utility::fcdb_state new_state) {
  check(new_state >= utility::fcdb_state::First
    && new_state <= utility::fcdb_state::Last, "wrong state to change to");
  
  // check bond exists
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw(), "dbond not found");

  // get dbond info
  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.find(dbond_id.raw());
    
  fcdb_stat.modify(fcdb_info, same_payer, [&](auto& stat) {
    stat.fc_state = (int)new_state;
  });

  if(utility::is_final_state(new_state)) {
    on_final_state(*fcdb_info);
  }
}

void dbonds::retire_fcdb(dbond_id_class dbond_id, extended_asset total_quantity_sent) {
  // it is supposed, that total_quantity_sent is on dbonds wallet already

  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw(), "dbond not found");

  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.get(dbond_id.raw());

  // check that the right token is sent to retire, ex. DUSD
  check(total_quantity_sent.get_extended_symbol() == fcdb_info.dbond.payoff_price.get_extended_symbol(),
    "to retire dbond you need to send the pay-off asset");

  if(has_auth(fcdb_info.dbond.emitent)) {
    check(fcdb_info.fc_state == (int)utility::fcdb_state::CIRCULATING, 
      "emitent can retire dbond only if it is in CIRCULATING state");

    // force buy off. fails if not enough amount is sent
    extended_asset left_after_retire = total_quantity_sent;
    for(name holder : fcdb_info.dbond.holders_list){
      force_retire_from_holder(dbond_id, holder, left_after_retire);
    }
    // transfer left_after_retire back to emitent if positive
    if(left_after_retire.quantity.amount != 0) {
      action(
        permission_level{_self, "active"_n},
        left_after_retire.contract, "transfer"_n,
        std::make_tuple(
          _self,
          fcdb_info.dbond.emitent,
          left_after_retire.quantity,
          string{"change for the retire of dbond "} + dbond_id.to_string())
      ).send();
    }

    // if succeed, all dbond supply is at emitent posession, dbond is at expired_paid_off state
    change_fcdb_state(dbond_id, utility::fcdb_state::EXPIRED_PAID_OFF);
  }

  else if(has_auth(fcdb_info.dbond.liquidation_agent)) {
    check(fcdb_info.fc_state == (int)utility::fcdb_state::EXPIRED_TECH_DEFAULTED,
      "dbond.liquidation_agent can call retire only at EXPIRED_TECH_DEFAULTED state");
    action(
      permission_level{_self, "active"_n},
      total_quantity_sent.contract, "transfer"_n,
      std::make_tuple(
        _self,
        fcdb_info.dbond.counterparty,
        total_quantity_sent.quantity,
        string{"retire by liquidation_agent dbond "} + dbond_id.to_string())
    ).send();
    change_fcdb_state(dbond_id, utility::fcdb_state::EXPIRED_PAID_OFF);
  }
  else
    check(false, "to retire you must be either dbond.emitent or dbond.liquidation_agent");
}

void dbonds::force_retire_from_holder(dbond_id_class dbond_id, name holder, extended_asset & left_after_retire) {
  stats statstable(_self, dbond_id.raw());
  const auto& st = statstable.get(dbond_id.raw());
  name emitent = st.issuer;
  // if holder is emitent || dBonds -> do nothing
  if(holder == _self || holder == emitent)
    return;

  // otherwise transfer dbond tokens holder->emitent and transfer appropriate payoff from dBonds
  asset dbonds_qtty = get_balance(_self, holder, dbond_id);
  sub_balance(holder, dbonds_qtty);
  add_balance(emitent, dbonds_qtty, _self);

  fc_dbond_index fcdb(_self, emitent.value);
  const auto& fcdb_info = fcdb.get(dbond_id.raw());
  extended_asset price = fcdb_info.dbond.payoff_price;
  int64_t payoff_amount = dbonds_qtty.amount * price.quantity.amount / utility::pow(10, price.quantity.symbol.precision());
  extended_asset payoff{{payoff_amount, price.quantity.symbol}, price.contract};
  action(
    permission_level{_self, "active"_n},
    payoff.contract, "transfer"_n,
    std::make_tuple(
      _self,
      holder,
      payoff.quantity,
      string{"payoff for the retire of dbond "} + dbond_id.to_string())
  ).send();

  // extract the paid off amount from left_after_retire
  left_after_retire -= payoff;
  // check that it is positive
  check(payoff.quantity.amount >= 0, "not enough assets to pay off for dbond retirement");
}
