#include <dbonds.hpp>
#include <utility.hpp>

#include <string>
#include <eosio/system.hpp>

void check_on_transfer(name from, name to, asset quantity, const string& memo){
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
  check(is_account(), "verifier account does not exist");

  SEND_INLINE_ACTION(*this, create, {{_this, "active"_n}}, {bond.emitent, bond.max_supply});

  stats statstable(_self, bond.bond_name.raw());
  auto dbond_stat = statstable.get(bond.bond_name.raw(), "dbond not found");

  fc_dbond_index fcdb_stat(_self, bond.emitent);
  auto existing = fcdb_stat.find(bond.bond_name.raw());

  if(existing == fcdb_stat.end()) {
    // new dbond, make a record for it
    fcdb_stat.emplace(bond.emitent, [&](auto& s) {
      s.dbond      = bond;
      s.verifier   = verifier;
      s.issue_time = time_point();
      s.fc_state   = utility::fc_dbond_state::CREATED;
    });
  } else if(existing->dbond.fc_state != utility::fc_dbond_state::CREATED) {
    // dbond already exists, but in state CREATED it may be overwritten
    fcdb_stat.modify(existing, bond.emitent, [&](auto& s) {
      s.dbond      = bond;
      s.verifier   = verifier;
    });
  } else {
    check(false, "dbond exists and not in CREATED state, change is not allowed");
  }

ACTION dbonds::verify(name from, dbond_id_class dbond_id) {
  
  require_auth(from);

  stats statstable(_self, dbond_id.raw());
  auto st = statstable.get(sym.code().raw(), "dbond not found");

  check(st.verifier == from, "you are not verifier");

  fc_dbond_index fcdb_stat(_self, st.dbond.emitent);
  auto fcdb_info = fcdb_stat.get(dbond_id.raw(), "CANNOT BE: dbond not found in fcdbond table");

  fcdb_stat.modify(fcdb_info, _self, [&](auto& s) {
    s.fc_state   = utility::fc_dbond_state::AGREEMENT_SIGNED;
  });
}
