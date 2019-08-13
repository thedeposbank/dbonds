#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include <utility.hpp>

using namespace std;
using namespace eosio;

struct fiat_bond {
  string ISIN;
  string name;
  string currency;
  string country;
  string bond_description_webpage;
};

using dbond_id_class = symbol_code;

struct dbond {
  dbond_id_class                   bond_name;               // unique name for dbond
  name                             emitent;                 // account who initiate an issue
  asset                            max_supply;              //

  extended_asset                   issue_price;           // unit of account, currency serves as price ex. DUSD
  time_point                       maturity_time;           // time until when to be paid off by emitent
  extended_asset                   payoff_price;           // price and currency for pay off
  bool                             fungible;                // if fungible or not


  string                           additional_info;
};

struct fc_dbond : dbond {
  fiat_bond                        collateral_bond;         // if collateral_type != FIAT_BOND then empty
  string                           escrow_contract_link
}

struct cc_dbond : dbond {
  extended_asset                   crypto_collateral;       // in case when collateral_type is CRYPTO_ASSET, this field stores asset
  int                              early_payoff_policy;    // if available, how is organized
}

