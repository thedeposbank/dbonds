#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <cstdlib>
#include <eosio/asset.hpp>
#include <eosio/name.hpp>

#define WEEK_uSECONDS microseconds(1000000LL*3600*24*7)

using namespace std;
using namespace eosio;

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
  
  int max_holders_number = 10;

  using dbond_id_class = symbol_code;

  bool match_memo(const string& memo, const string& pattern, dbond_id_class& bond_name) {
    auto pos = memo.find(pattern);
    if(pos == string::npos || pos != 0) return false;
    bond_name = symbol_code();
    if(memo.size() == pattern.size()) return true;
    string name_str = memo.substr(pattern.size());
    bond_name = symbol_code(name_str);
    return true;
  }

  uint64_t pow(uint64_t x, uint64_t p) {
    if(p == 0)
      return 1;
    if(p & 1) {
      return x * pow(x, p-1);
    }
    else {
      uint64_t res = pow(x, p/2);
      return res * res;
    }
  }
  // enum class early_payoff_policy: int {
  //   FULL_INTEREST_RATE = 0,
  //   TIME_LINEAR_INTEREST_RATE = 1,          // not supported yet
  //   First = FULL_INTEREST_RATE,
  //   Last = TIME_LINEAR_INTEREST_RATE
  // };
  
  // struct MEMOS {
  //   const string put_collateral = "put collateral ";
  //   const string buy_bond = "buy bond ";
  //   const string payoff_bond = "pay off bond ";
  //   const string exchange = "exchange";
  // } memos;

  // enum class cc_dbond_state: int {
  //   CREATION = 0,
  //   INITIAL_SALE_OFFER = 1,
  //   CIRCULATING = 2,
  //   CIRCULATING_PAID_OFF = 3,
  //   EXPIRED_PAID_OFF = 4,
  //   DEFAULTED = 5,
  //   EMPTY = 6,
  //   First = CREATION,
  //   Last = EMPTY
  // };
  
  // // this table demonstrates how state can be changed
  // // state_graph[i][j]=1 means it is allowed to change state from i to j
  // // this is mostly for description. it is used in code as double check, 
  // // though it is not necessary
  // constexpr bool state_graph[7][7] = {
  //   {0,1,0,0,0,0,0},
  //   {1,0,1,0,0,0,0},
  //   {0,0,0,1,0,1,0},
  //   {0,0,0,1,1,0,1},
  //   {0,0,0,0,0,0,1},
  //   {0,0,0,0,0,0,1},
  //   {0,0,0,0,0,0,0},
  // };

} // namespace utility
