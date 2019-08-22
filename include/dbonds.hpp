#pragma once

#include <dbond.hpp>

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>

const name DBVERIFIER("fcdbverifier");

using namespace eosio;
using namespace std;

CONTRACT dbonds : public contract {
public:
  using contract::contract;
  
  // classic token actions
  ACTION transfer(name from, name to, asset quantity, const string& memo);

  ACTION create(name issuer, asset maximum_supply);

  ACTION issue(name to, asset quantity, string memo);

  // dbond actions
  ACTION initfcdb(fc_dbond & bond);

  ACTION verifyfcdb(name from, dbond_id_class dbond_id);

  ACTION issuefcdb(name from, dbond_id_class dbond_id);
  
  ACTION burn(name from, dbond_id_class dbond_id);

  ACTION updfcdb(dbond_id_class dbond_id);

  ACTION confirmfcdb(dbond_id_class dbond_id);

  

#ifdef DEBUG    
  ACTION erase(name owner, dbond_id_class dbond_id);
  ACTION setstate(dbond_id_class dbond_id, int state);
#endif

  [[eosio::on_notify("*::transfer")]]
  void ontransfer(name from, name to, asset quantity, const string& memo);

private:
  
  // scope: same as primary key (dbond id)
  TABLE currency_stats {
    asset          supply;
    asset          max_supply;
    name           issuer;

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
  };

  // scope: user name (current dbond owner)
  TABLE account {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
  };

  // scope: dbond.emitent
  TABLE fc_dbond_stats {
    fc_dbond             dbond;
    time_point           initial_time;
    extended_asset       initial_price;
    extended_asset       current_price;
    int                  fc_state;
    int                  confirmed_by_counterparty;

    uint64_t primary_key() const { return dbond.bond_name.raw(); }
  };

  // TABLE cc_dbond_stats {
  //   dbond_id_class  dbond_id;
    
  //   uint64_t primary_key() const { return dbond_id.raw(); }
  // };

  // TABLE nc_dbond_stats {
  //   dbond_id_class  dbond_id;
    
  //   uint64_t primary_key() const { return dbond_id.raw(); }
  // };

  using stats          = multi_index< "stat"_n, currency_stats >;
  using accounts       = multi_index< "accounts"_n, account >;
  using fc_dbond_index = multi_index< "fcdbond"_n, fc_dbond_stats >;
  // using cc_dbond_index = multi_index< "ccdbond"_n, cc_dbond_stats >;
  // using nc_dbond_index = multi_index< "ncdbond"_n, nc_dbond_stats >;

  static asset get_supply(name token_contract_account, symbol_code sym_code)
  {
    stats statstable(token_contract_account, sym_code.raw());
    const auto& st = statstable.get(sym_code.raw());
    return st.supply;
  }

  static asset get_balance(name token_contract_account, name owner, symbol_code sym_code)
  {
    stats statstable(token_contract_account, sym_code.raw());
    const auto& st = statstable.get(sym_code.raw());

    accounts accountstable(token_contract_account, owner.value);
    auto ac = accountstable.find(sym_code.raw());
    if(ac == accountstable.end()) {
      return {0, st.max_supply.symbol};
    }
    return ac->balance;
  }

  void change_fcdb_state(dbond_id_class dbond_id, utility::fcdb_state new_state);
  void sub_balance(name owner, asset value);
  void add_balance(name owner, asset value, name ram_payer);
  void check_on_transfer(name from, name to, asset quantity, const string& memo);
  void check_on_fcdb_transfer(name from, name to, asset quantity, const string& memo);
  void check_fcdb_sanity(const fc_dbond& bond);
  void set_initial_data(dbond_id_class dbond_id);
  
  void retire_fcdb(dbond_id_class dbond_id, extended_asset total_quantity_sent);
  void on_successful_retire(dbond_id_class dbond_id);
  void process_retire_by_liquidation_agent(dbond_id_class dbond_id, extended_asset total_quantity_sent);
  void force_retire_from_holder(dbond_id_class dbond_id, name holder, extended_asset & left_after_retire);
  void collect_fcdb_on_dbonds_account(dbond_id_class dbond_id);
};
