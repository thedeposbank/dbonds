#pragma once

#include <dbond.hpp>

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>

using namespace eosio;
using namespace std;

CONTRACT dbonds : public contract {
  public:
    using contract::contract;
    
    // classic token actions
    ACTION transfer(name from, name to, asset quantity, const string& memo);

    // dbond actions
    ACTION create(name from, dbond bond);
    
    ACTION burn(name from, dbond_id_class dbond_id);
    
    ACTION listsale(name from, dbond_id_class dbond_id);
    
    ACTION cancelsale(name from, dbond_id_class dbond_id);
    
    ACTION expire(dbond_id_class dbond_id);

    ACTION updcurprice(dbond_id_class dbond);

#ifdef DEBUG    
    ACTION erase(name owner, dbond_id_class dbond_id);
#endif

    [[eosio::on_notify("*::transfer")]] // change to *::transfer
    void ontransfer(name from, name to, asset quantity, const string& memo);

    // eosio.cdt bug workaround
    [[eosio::on_notify("dummy1234512::transfer")]]
    void dummy(name from, name to, asset quantity, const string& memo) {}

  private:
  
    TABLE currency_stats {
      asset          supply;
      asset          max_supply;
      name           issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };

    TABLE account {
      asset balance;

      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    TABLE asks {
      dbond_id_class  dbond_id;
      name            seller;
      asset           amount;

      uint64_t primary_key() const { return dbond_id.raw(); }
    };

    TABLE fc_dbond_stats {
      dbond_id_class  dbond_id;

      dbond                dbond;
      time_point           initial_sale_time;
      uint64_t             current_price;
      int                  fc_state;

      uint64_t primary_key() const { return dbond_id.raw(); }
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
    using asks_index     = multi_index< "asks"_n, asks >;
    using fc_dbond_index = multi_index< "fcdbond"_n, fc_dbond_stats >;
    // using cc_dbond_index = multi_index< "ccdbond"_n, cc_dbond_stats >;
    // using nc_dbond_index = multi_index< "ncdbond"_n, nc_dbond_stats >;



    void changestate(dbond_id_class dbond_id, int state);
    void check_dbond_sanity(const dbond& bond);
    void sub_balance(name owner, asset value);
    void add_balance(name owner, asset value, name ram_payer);
};
