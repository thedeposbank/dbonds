#include <dbonds.hpp>
#include <utility.hpp>

#include <string>
#include <eosio/system.hpp>

ACTION dbonds::transfer(name from, name to, asset quantity, const string& memo) {
  check(from != to, "cannot transfer to self");
  require_auth(from);
  check(is_account(to), "to account does not exist");
  auto sym = quantity.symbol.code();
  stats statstable(_self, sym.raw());
  const auto& st = statstable.get(sym.raw(), "no stats for given symbol code 4");

  require_recipient(from);
  require_recipient(to);
  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount == 1 && quantity.symbol.precision() == 0, "quantity must be 1");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  auto payer = has_auth(to) ? to : from;

  sub_balance(from, quantity);
  add_balance(to, quantity, payer);

  dbond_id_class bond_name;
  if(to == get_self() && utility::match_memo(memo, utility::memos.exchange, bond_name)) {
    exchange(from, quantity.symbol.code());
  }
}

ACTION dbonds::create(name from, dbond dbond) {
	require_auth(from);

	stats statstable(get_self(), get_self().value);
  auto existing_bond = statstable.find(dbond.bond_name.raw());
  check(existing_bond == statstable.end(), "Bond with bond_name exists");
  check(dbond.emitent == from, "bond.emitent should be the one who makes an action");
  check_dbond_sanity(dbond);

	statstable.emplace(from, [&](auto& stats) {
    stats.dbond                 = dbond;
    stats.got_crypto_collateral = false;
    stats.supply                = asset(0, symbol(dbond.bond_name,0));
    stats.max_supply            = asset(1, symbol(dbond.bond_name,0));
    stats.issuer                = dbond.emitent;
    stats.state                 = int(utility::dbond_state::CREATION);
	});
}

ACTION dbonds::ontransfer(name from, name to, asset quantity, const string& memo) {

  extended_asset ext_quantity(quantity, get_first_receiver());
  if(to != get_self() || get_first_receiver() == get_self()) {
    return;
  }

  dbond_id_class bond_name;
  if(utility::match_memo(memo, utility::memos.put_collateral, bond_name)) {

    sendintobond(from, ext_quantity, bond_name);

  } else
  if(utility::match_memo(memo, utility::memos.buy_bond, bond_name)) {

    buy(from, bond_name, ext_quantity);

  } else
  if(utility::match_memo(memo, utility::memos.payoff_bond, bond_name)) {

    payoff(from, bond_name, ext_quantity);

  } else {
    check(false, "wrong memo format");
  }
}

void dbonds::sendintobond(name from, extended_asset collateral, dbond_id_class dbond_id) {
  
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "bond with given name not found");

  // assert collateral == collateral claimed in dbond
  check(st.dbond.collateral_type == int(utility::collateral_type::CRYPTO_ASSET),
    "bond collateral type is not crypto!");
  // check(st.dbond.collateral_type == int(utility::collateral_type::CRYPTO_ASSET),
  //   "bond collateral type is not crypto!");
  check(st.dbond.crypto_collateral == collateral, "wrong collateral");

  // assert from == bond.emitent && to == dbonds
  check(st.dbond.emitent == from, "only emitent can send collateral");
  
  // assert dbond does not have collateral inside (double collaterization)
  check(!st.got_crypto_collateral, "collateral is acquired already");
  
  // assert dbond.state == CREATION
  check(st.state == int(utility::dbond_state::CREATION),
    "dbond state must be " + to_string(int(utility::dbond_state::CREATION)));
  
  // lock collateral in dbonds under dbond_id
  statstable.modify(st, same_payer, [&](auto& stat) {
    stat.got_crypto_collateral = true;
  });
}

ACTION dbonds::change(dbond_id_class dbond_id, dbond dbond) {
  
  //assert from = dbond.emitent
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");
  require_auth(st.dbond.emitent);

  print("current dbond state: ", st.state);
  
  check(st.state == int(utility::dbond_state::CREATION)
    || st.state == int(utility::dbond_state::INITIAL_SALE_OFFER),
    "dbond state must be " + to_string(int(utility::dbond_state::CREATION)) +
    " or " + to_string(int(utility::dbond_state::INITIAL_SALE_OFFER)));

  check_dbond_sanity(dbond);

  // when in state INITIAL_SALE_OFFER, accept all data fields
  if(st.state == int(utility::dbond_state::INITIAL_SALE_OFFER)) {
    statstable.modify(st, same_payer, [&](auto& stat) {
      extended_asset crypto_collateral_backup = stat.dbond.crypto_collateral;
      stat.dbond = dbond;
      // if collateral is locked inside, change does not contain collateral
      if(st.got_crypto_collateral) {
        stat.dbond.crypto_collateral = crypto_collateral_backup;
      }
    });
  // when in state CREATION, accept only price
  } else {
    statstable.modify(st, same_payer, [&](auto& stat) {
      stat.dbond.issue_price = dbond.issue_price;
    });
  }
}

ACTION dbonds::burn(name from, dbond_id_class dbond_id) {
  
  require_auth(from);
  
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");
  
  print("current dbond state: ", st.state);
  
  // assert dbond.state == CREATION || dbond.state == EMPTY
  check(st.state == int(utility::dbond_state::CREATION)
    || st.state == int(utility::dbond_state::EMPTY), 
    "dbond.state must be " + to_string(int(utility::dbond_state::CREATION))
    + " or " + to_string(int(utility::dbond_state::EMPTY)));
  
  // if dbond.state == CREATION then assert from == dbond.emitent
  if(st.state == int(utility::dbond_state::CREATION))
    check(from == st.dbond.emitent, "at this state only emitent can burn dbond");
  
  // if there is already a collateral behind the bond, send it back to emitent
  if(st.got_crypto_collateral) {
    action( 
      permission_level{ get_self(), name("active") },
      name(st.dbond.crypto_collateral.contract),
      name("transfer"),
      make_tuple(
        get_self(),
        st.dbond.emitent,
        st.dbond.crypto_collateral.quantity,
        "payoff of dbond " + dbond_id.to_string())
    ).send();
  }
  
  // erase bond stat record
  statstable.erase(st);
  // erase bond balance record if exists (state EMPTY, on issuer's balance)
  // this works for nft only!
  accounts acnts(_self, _self.value);
  const auto& balance = acnts.find(dbond_id.raw());
  if(balance != acnts.end()) {
    acnts.erase(balance);
  }
}

ACTION dbonds::listsale(name from, dbond_id_class dbond_id) {
  
  require_auth(from);

  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");
  
  print("current dbond state: ", st.state);

  // assert dbond.state == CREATION
  check(st.state == int(utility::dbond_state::CREATION),
    "dbond state must be " + to_string(int(utility::dbond_state::CREATION)));
  
  // assert dbond has collateral as claimed
  if(st.dbond.collateral_type == int(utility::collateral_type::CRYPTO_ASSET)) {
    check(st.got_crypto_collateral, "dbond must have collateral");
  }
  
  // assert all dbond fields are valid (not empty, right format)
  check_dbond_sanity(st.dbond);
  
  // list dbond in ASKS table
  asks_index asks_table(_self, _self.value);
  const auto& existing_ask = asks_table.find(dbond_id.raw());
  check(existing_ask == asks_table.end(), "dbond already listed?");

  asks_table.emplace(from, [&](auto& ask) {
    ask.dbond_id = dbond_id;
    ask.seller = from;
    ask.amount = asset(1, symbol(dbond_id, 0));
  });
  
  // change dbond state to INITIAL_SALE_OFFER
  statstable.modify(st, same_payer, [&](auto& stat) {
    stat.state = int(utility::dbond_state::INITIAL_SALE_OFFER);
  });
}

void dbonds::buy(name buyer, dbond_id_class dbond_id, extended_asset price) {
  
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");

  print("current dbond state: ", st.state);

  // assert dbond.state == INITIAL_SALE_OFFER
  check(st.state == int(utility::dbond_state::INITIAL_SALE_OFFER),
    "dbond state must be " + to_string(int(utility::dbond_state::INITIAL_SALE_OFFER)));

  check(price == st.dbond.issue_price, "wrong price");
  
  // transfer dbond to buyer. actually, just increment balance
  add_balance(buyer, {1, {dbond_id, 0}}, get_self());

  // add supply, write initial sale time
  statstable.modify(st, same_payer, [&](auto& stat) {
    stat.supply = {1, {dbond_id, 0}};
    stat.initial_sale_time = current_time_point();
  });

  // delete ask
  asks_index asks(get_self(), get_self().value);
  auto& ask = asks.get(dbond_id.raw(), "no ask for dbond in state 'INITIAL_SALE_OFFER'");
  asks.erase(ask);

  // transfer price to bond.emitent
  action( 
    permission_level{ get_self(), name("active") },
    name(price.contract),
    name("transfer"),
    make_tuple(
      get_self(),
      st.dbond.emitent,
      price.quantity,
      "received from sale " + dbond_id.to_string())
  ).send();

  // change dbond.state to CIRCULATING
  changestate(dbond_id, int(utility::dbond_state::CIRCULATING));
}

ACTION dbonds::cancelsale(name from, dbond_id_class dbond_id) {
  
  require_auth(from);

  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");

  print("current dbond state: ", st.state);
  
  // assert dbond.state == INITIAL_SALE_OFFER
  check(st.state == int(utility::dbond_state::INITIAL_SALE_OFFER), 
      "dbond state must be " + to_string(int(utility::dbond_state::INITIAL_SALE_OFFER)));
  
  // assert from == dbond.emitent
  check(from == st.dbond.emitent, "only dbond.emitent can call this action");
  
  // change dbond state to CREATION
  changestate(dbond_id, int(utility::dbond_state::CREATION));
  
  // delete dbond from ASKS table
  asks_index ask_table( get_self(), get_self().value );
  const auto& ask = ask_table.get( dbond_id.raw(), "cannot cancel sale that doesn't exist" );
  ask_table.erase(ask);
}

void dbonds::changestate(dbond_id_class dbond_id, int state) {

  check(state >= int(utility::dbond_state::First)
    && state <= int(utility::dbond_state::Last), "wrong state to change to");
  
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");
 
  // send warning or, at first stage maybe even exception, if // TODO: clarify
  // max(state, bond.state) - min(state, dbond.state) > 1
  check(utility::state_graph[st.state][state], "tried to change dbond.state from "
    + to_string(st.state)
    + " to " + to_string(state) + ", which is wrong");
    
  statstable.modify(st, same_payer, [&](auto& stat) {
    stat.state = state;
  });
}

void dbonds::payoff(name from, dbond_id_class dbond_id, extended_asset for_payoff) {
  
  stats statstable(get_self(), get_self().value);
  const auto& st = statstable.get(dbond_id.raw(), "dbond with given name not found");
  
  print("current dbond state: ", st.state);

  // assert dbond.state == CIRCULATING
  check(st.state == int(utility::dbond_state::CIRCULATING),
    "dbond state must be " + to_string(int(utility::dbond_state::CIRCULATING)));
  
  check(for_payoff == st.dbond.payoff_price, "payoff payment must match dbond.payoff_price exactly");
  
  // if there is a collateral send it back to dbond.emitent
  if(st.got_crypto_collateral) {
    action( 
      permission_level{ get_self(), name("active") },
      name(st.dbond.crypto_collateral.contract),
      name("transfer"),
      make_tuple(
        get_self(),
        st.dbond.emitent,
        st.dbond.crypto_collateral.quantity,
        "payoff of dbond " + dbond_id.to_string())
    ).send();
  }
  
  changestate(dbond_id, int(utility::dbond_state::CIRCULATING_PAID_OFF));
}

void dbonds::exchange(name from, dbond_id_class dbond_id) {
  
  stats statstable(get_self(), get_self().value);
  // check for dbond existence
  const auto& st = statstable.get(dbond_id.raw(), "bond with given name not found");

  print("current dbond state: ", st.state);

  check(st.state == int(utility::dbond_state::CIRCULATING_PAID_OFF)
    || st.state == int(utility::dbond_state::EXPIRED_PAID_OFF)
    || st.state == int(utility::dbond_state::DEFAULTED), "dbond state must be "
    + to_string(int(utility::dbond_state::CIRCULATING_PAID_OFF)) + ", "
    + to_string(int(utility::dbond_state::EXPIRED_PAID_OFF)) + " or "
    + to_string(int(utility::dbond_state::DEFAULTED)));
  
  // if there is a collateral or payoff (according to logic it either
  // one of them or none, not both) send it to "from".
  if(st.state == int(utility::dbond_state::DEFAULTED)) {
    // exchange for collateral
    if(st.dbond.collateral_type == int(utility::collateral_type::CRYPTO_ASSET)) {
      check(st.got_crypto_collateral, "got no collateral!");
      action( 
        permission_level{ get_self(), name("active") },
        name(st.dbond.crypto_collateral.contract),
        name("transfer"),
        make_tuple(
          get_self(),
          from,
          st.dbond.crypto_collateral.quantity,
          "exchange " + dbond_id.to_string() + " for collateral")
      ).send();
    }
  } else {
    // exchange for payoff
    action( 
      permission_level{ get_self(), name("active") },
      name(st.dbond.payoff_price.contract),
      name("transfer"),
      make_tuple(
        get_self(),
        from,
        st.dbond.payoff_price.quantity,
        "exchange " + dbond_id.to_string() + " for payoff")
    ).send();
  }

  //change got_collateral to 'false'
  statstable.modify(st, same_payer, [&](auto& stat){
    stat.got_crypto_collateral = false;
  });
  
  // change dbond.state to EMPTY
  changestate(dbond_id, int(utility::dbond_state::EMPTY));  
}

#ifdef DEBUG
ACTION dbonds::erase(name owner, dbond_id_class dbond_id) {
  stats statstable(get_self(), get_self().value);
  const auto st = statstable.find(dbond_id.raw());
  if(st != statstable.end()) {
    statstable.erase(st);
  }
  asks_index asks(get_self(), get_self().value);
  const auto ask = asks.find(dbond_id.raw());
  if(ask != asks.end()) {
    asks.erase(ask);
  }
  accounts acnts(get_self(), owner.value);
  const auto account = acnts.find(dbond_id.raw());
  if(account != acnts.end()) {
    acnts.erase(account);
  }
  accounts acnts2(get_self(), _self.value);
  const auto account2 = acnts2.find(dbond_id.raw());
  if(account2 != acnts2.end()) {
    acnts2.erase(account2);
  }
}
#endif

ACTION dbonds::expire(dbond_id_class dbond_id) {
  
  stats statstable(get_self(), get_self().value);
  // check for dbond existence
  const auto& st = statstable.get(dbond_id.raw(), "bond with given name not found");

  print("current dbond state: ", st.state);

#ifdef DEBUG
  auto maturity_time = time_point(microseconds(0));
#else
  auto maturity_time = st.dbond.maturity_time;
#endif

  if(current_time_point() >= maturity_time) {
    switch(st.state) {

      case int(utility::dbond_state::INITIAL_SALE_OFFER):
        // change dbond state to CREATION
        changestate(dbond_id, int(utility::dbond_state::CREATION));
        { // delete dbond from ASKS table
          asks_index ask_table( get_self(), get_self().value );
          const auto& ask = ask_table.find( dbond_id.raw());
          if(ask != ask_table.end()) ask_table.erase(ask);
        }
        break;

      case int(utility::dbond_state::CIRCULATING):
        // change dbond state to DEFAULTED
        changestate(dbond_id, int(utility::dbond_state::DEFAULTED));
        break;

      case int(utility::dbond_state::CIRCULATING_PAID_OFF):
        // change dbond state to EXPIRED_PAID_OFF
        changestate(dbond_id, int(utility::dbond_state::EXPIRED_PAID_OFF));
        break;

      default:
        // wrong dbond state
        check(false, "wrong dbond state");
    }
  }
}

void dbonds::check_dbond_sanity(const dbond& bond) {
  check(bond.maturity_time >= current_time_point() + WEEK_uSECONDS, 
    "maturity_time must be at least a week in future from now");
  
  if(bond.issue_price.quantity.symbol == bond.payoff_price.quantity.symbol
    && bond.issue_price.contract == bond.payoff_price.contract) {
    check(bond.issue_price.quantity <= bond.payoff_price.quantity,
      "issue price must be lower than pay off price");
  }
  check(bond.max_supply == asset(1, symbol(bond.bond_name, 0)),
    "for nft token max_supply must be 1");

  check(bond.collateral_type >= int(utility::collateral_type::First)
    && bond.collateral_type <= int(utility::collateral_type::Last),
    "collateral type out of range");

  check(!bond.fungible, "there is no support for fungible bonds at the moment");

  check(bond.early_payoff_policy == int(utility::early_payoff_policy::FULL_INTEREST_RATE),
    "wrong type of early pay off policy");

  if(bond.collateral_type == int(utility::collateral_type::CRYPTO_ASSET)) {
    check(bond.crypto_collateral.quantity.is_valid()
      && bond.crypto_collateral.quantity.amount > 0,
      "crypto collateral quantity must be positive");
  }
}

void dbonds::sub_balance(name owner, asset value)
{
  accounts from_acnts(_self, owner.value);

  const auto& from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  name ram_payer = owner;
  from_acnts.modify( from, ram_payer, [&](auto& a) {
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

