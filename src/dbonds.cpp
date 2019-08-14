#include <dbonds.hpp>
#include <utility.hpp>

#include <string>
#include <eosio/system.hpp>

void dbonds::check_on_transfer(name from, name to, asset quantity, const string& memo) {
  check(from != to, "cannot transfer to self");
  require_auth(from);
  check(is_account(to), "to account does not exist");
  auto sym = quantity.symbol.code();
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(sym.raw(), "no stats for given symbol code");

  require_recipient(from);
  require_recipient(to);
  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount == 1 && quantity.symbol.precision() == 0, "quantity must be 1");
  check(memo.size() <= 256, "memo has more than 256 bytes");
}

ACTION dbonds::transfer(name from, name to, asset quantity, const string& memo) {
  
  check_on_transfer(from, to, quantity, memo);
  
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

  statstable.emplace(_self, [&]( auto& s ) {
    s.supply.symbol = maximum_supply.symbol;
    s.max_supply    = maximum_supply;
    s.issuer        = issuer;
  });

}

ACTION dbonds::initfcdb(fc_dbond bond, name verifier) {

  require_auth(bond.emitent);
  check(is_account(verifier), "verifier account does not exist");

  SEND_INLINE_ACTION(*this, create, {{_self, "active"_n}}, {bond.emitent, bond.max_supply});

  stats statstable(_self, bond.bond_name.raw());
  auto dbond_stat = statstable.get(bond.bond_name.raw(), "dbond not found");

  fc_dbond_index fcdb_stat(_self, bond.emitent.value);
  auto existing = fcdb_stat.find(bond.bond_name.raw());

  if(existing == fcdb_stat.end()) {
    // new dbond, make a record for it
    fcdb_stat.emplace(bond.emitent, [&](auto& s) {
      s.dbond      = bond;
      s.verifier   = verifier;
      s.issue_time = time_point();
      s.fc_state   = (int)utility::fc_dbond_state::CREATED;
    });
  } else if(existing->fc_state != (int)utility::fc_dbond_state::CREATED) {
    // dbond already exists, but in state CREATED it may be overwritten
    fcdb_stat.modify(existing, bond.emitent, [&](auto& s) {
      s.dbond      = bond;
      s.verifier   = verifier;
    });
  } else {
    check(false, "dbond exists and not in CREATED state, change is not allowed");
  }
}

ACTION dbonds::verify(name from, dbond_id_class dbond_id) {
  
  require_auth(from);

  stats statstable(_self, dbond_id.raw());
  auto st = statstable.get(dbond_id.raw(), "dbond not found");

  fc_dbond_index fcdb_stat(_self, st.issuer.value);
  auto fcdb_info = fcdb_stat.get(dbond_id.raw(), "CANNOT BE: dbond not found in fcdbond table");

  check(fcdb_info.verifier == from, "you are not verifier");

  fcdb_stat.modify(fcdb_info, _self, [&](auto& s) {
    s.fc_state   = (int)utility::fc_dbond_state::AGREEMENT_SIGNED;
  });
}

ACTION dbonds::issuefcdb(name from, dbond_id_class dbond_id) {}
ACTION dbonds::burn(name from, dbond_id_class dbond_id) {}
void dbonds::ontransfer(name from, name to, asset quantity, const string& memo) {}

//////////////////////////////////////////////////////////

void dbonds::sub_balance(name owner, asset value)
{
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

void dbonds::add_balance(name owner, asset value, name ram_payer)
{
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
