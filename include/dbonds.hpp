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
      
    ACTION change(dbond_id_class dbond_id, dbond dbond);
    
    ACTION burn(name from, dbond_id_class dbond_id);
    
    ACTION listsale(name from, dbond_id_class dbond_id);
    
    ACTION cancelsale(name from, dbond_id_class dbond_id);
    
    ACTION expire(dbond_id_class dbond_id);

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

      dbond                dbond;
      bool                 got_crypto_collateral;
      time_point           initial_sale_time;
      int                  state;

      uint64_t primary_key() const { return dbond.bond_name.raw(); }
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
    
    using stats = multi_index< "stat"_n, currency_stats >;
    using accounts = multi_index< "accounts"_n, account >;
    using asks_index = multi_index< "asks"_n, asks >;

    void sendintobond(name from, extended_asset collateral, dbond_id_class dbond_id);
    void exchange(name from, dbond_id_class dbond_id);
    void buy(name buyer, dbond_id_class dbond_id, extended_asset price);
    void payoff(name from, dbond_id_class dbond_id, extended_asset for_payoff);

    void changestate(dbond_id_class dbond_id, int state);
    void check_dbond_sanity(const dbond& bond);
    void sub_balance(name owner, asset value);
    void add_balance(name owner, asset value, name ram_payer);
};
