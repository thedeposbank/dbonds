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

