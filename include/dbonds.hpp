#pragma once

#include "dbond.hpp"

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>

const name DBVERIFIER("fcdbverifier");

using namespace eosio;
using namespace std;

namespace utility {

  enum class fcdb_state: int {
    CREATED = 0,
    AGREEMENT_SIGNED = 1,
    CIRCULATING = 2,
    EXPIRED_PAID_OFF = 3, // once this status is set, dbond.holders_list = [dBonds, emitent]
    EXPIRED_TECH_DEFAULTED = 4,
    EXPIRED_DEFAULTED = 5,
    First = CREATED,
    Last = EXPIRED_DEFAULTED
  };
  
  bool is_final_state(utility::fcdb_state state){
    return state == fcdb_state::EXPIRED_PAID_OFF || state == fcdb_state::EXPIRED_DEFAULTED;
  }
}

CONTRACT dbonds : public contract {
public:
  using contract::contract;
  
  // classic token actions
  ACTION transfer(name from, name to, asset quantity, const string& memo);

  ACTION create(name issuer, asset maximum_supply);

  ACTION issue(name to, asset quantity, string memo);

  // dbond actions
  ACTION initfcdb(const fc_dbond& bond);

  ACTION verifyfcdb(name from, dbond_id_class dbond_id);

  ACTION issuefcdb(name from, dbond_id_class dbond_id);
  
  ACTION burn(name from, dbond_id_class dbond_id);

  ACTION updfcdb(dbond_id_class dbond_id);

  ACTION confirmfcdb(dbond_id_class dbond_id);

  ACTION del(dbond_id_class dbond_id);

  ACTION listprivord(dbond_id_class dbond_id, name seller, name buyer, extended_asset recieved_asset, bool is_sell);

#ifdef DEBUG    
  ACTION erase(vector<name> holders, dbond_id_class dbond_id);
  ACTION setstate(dbond_id_class dbond_id, int state);
private:
  template<class T>
  int erase_table(int64_t scope) {
    int count = 0;
    T table(_self, scope);
    for(auto itr = table.begin(); itr != table.end();)
      itr = table.erase(itr);
    return count;
  }
public:
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

    uint64_t primary_key() const { return dbond.dbond_id.raw(); }
  };

  // TABLE cc_dbond_stats {
  //   dbond_id_class  dbond_id;
    
  //   uint64_t primary_key() const { return dbond_id.raw(); }
  // };

  // TABLE nc_dbond_stats {
  //   dbond_id_class  dbond_id;
    
  //   uint64_t primary_key() const { return dbond_id.raw(); }
  // };

  // scope: dbond_id
  TABLE fc_dbond_order_struct {
    name           seller;
    name           buyer;
    extended_asset recieved_payment;
    asset          recieved_quantity;
    extended_asset price;

    uint64_t primary_key() const { return seller.value; }
    uint128_t secondary_key_1() const { return concat128(seller.value, buyer.value); }

  };

  using stats             = multi_index< "stat"_n, currency_stats >;
  using accounts          = multi_index< "accounts"_n, account >;
  using fc_dbond_index    = multi_index< "fcdbond"_n, fc_dbond_stats >;
  // using cc_dbond_index = multi_index< "ccdbond"_n, cc_dbond_stats >;
  // using nc_dbond_index = multi_index< "ncdbond"_n, nc_dbond_stats >;
  using fc_dbond_orders   = multi_index<
    "fcdborders"_n,
    fc_dbond_order_struct,
    indexed_by< "peers"_n, const_mem_fun<fc_dbond_order_struct, uint128_t, &fc_dbond_order_struct::secondary_key_1> > >;

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

  static uint128_t concat128(uint64_t x, uint64_t y) {
    return ((uint128_t)x << 64) + (uint128_t)y;
  }

  void change_fcdb_state(dbond_id_class dbond_id, utility::fcdb_state new_state);
  void sub_balance(name owner, asset value);
  void add_balance(name owner, asset value, name ram_payer);
  void check_on_transfer(name from, name to, asset quantity, const string& memo);
  void check_on_fcdb_transfer(name from, name to, asset quantity, const string& memo);
  void check_fcdb_sanity(const fc_dbond& bond);
  void set_initial_data(dbond_id_class dbond_id);
  
  void retire_fcdb(dbond_id_class dbond_id, extended_asset total_quantity_sent);
  void force_retire_from_holder(dbond_id_class dbond_id, name holder, extended_asset & left_after_retire);
  void collect_fcdb_on_dbonds_account(dbond_id_class dbond_id);
  void erase_dbond(dbond_id_class dbond_id);
  void on_final_state(const fc_dbond_stats& fcdb_info);
  void register_private_order_fcdb(dbond_id_class dbond_id, name seller, name buyer, extended_asset recieved_asset, bool is_sell);
  void match_trade(dbond_id_class dbond_id, name seller, name buyer);

};
