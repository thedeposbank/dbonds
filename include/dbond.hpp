#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <string>
#include <vector>

using namespace std;
using namespace eosio;

struct fiat_bond {
  string ISIN;
  string name;
  string issuer;
  string currency;
  time_point maturity_time;
  string bond_description_webpage;
};

using dbond_id_class = symbol_code;

struct dbond {
  dbond_id_class                   dbond_id;               // unique name for dbond
  name                             emitent;                 // account who initiate an issue
  asset                            quantity_to_issue;


  time_point                       maturity_time;           // time until when to be paid off by emitent
  time_point                       retire_time;             // when tech. defaulted dbond becomes defaulted
  extended_asset                   payoff_price;            // price and currency for pay off
  bool                             fungible;                // if fungible or not

  string                           additional_info;
};

struct fc_dbond : dbond {
  fiat_bond                        collateral_bond;         // 
  name                             verifier;
  name                             counterparty;
  name                             liquidation_agent;       // the one responsible for handling the fiat assets in case of default
  string                           escrow_contract_link;
  int64_t                          apr;                     // in format where 1000 meaning 10%
  vector<name>                     holders_list;            // list of accounts, any other cannot obtain the dbond
};

struct cc_dbond : dbond {
  extended_asset                   crypto_collateral;       // in case when collateral_type is CRYPTO_ASSET, this field stores asset
  int                              early_payoff_policy;     // if available, how is organized
  asset                            max_supply;              //
  extended_asset                   issue_price;             // unit of account, currency serves as price ex. DUSD
};