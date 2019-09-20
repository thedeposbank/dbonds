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

  int max_holders_number = 10;

  using dbond_id_class = symbol_code;

  bool match_icase(const string& memo, const string& pattern) {
    auto i1 = memo.begin();
    auto i2 = pattern.begin();
    for(; i1 != memo.end() && i2 != pattern.end(); i1++, i2++)
      if(tolower(*i1) != tolower(*i2))
        return false;
    return i1 == memo.end() && i2 == pattern.end();
  }

  /*
   * match string to pattern from the beginning, treat rest of string as dbond_id
   */
  bool match_memo(const string& memo, const string& pattern, dbond_id_class& dbond_id) {
    auto i1 = memo.begin();
    auto i2 = pattern.begin();
    for(; i1 != memo.end() && i2 != pattern.end(); i1++, i2++)
      if(tolower(*i1) != tolower(*i2))
        return false;
    dbond_id = symbol_code();
    if(i1 == memo.end() && i2 == pattern.end())
      return true;
    string name_str = string{i1, memo.end()};
    dbond_id = symbol_code(name_str);
    return true;
  }

  bool valid_dbond_char(int c) {
    return c >= 'A' && c <= 'Z';
  }

  bool valid_name_char(int c) {
    return (c >= 'a' && c <= 'z') || (c >= '1' && c <= '5');
  }

  /*
   * match string to pattern, trying to treat first "?" as dbond_id, second "?" -- as account name
   */

  bool match_memo(const string& memo, const string& pattern, dbond_id_class& dbond_id, name& who) {
    // expected memo: "buy DBONDA from thedeposbank" || "sell DBONDA to thedeposbank"
    string tokens[4] = {"", "", "", ""};
    string cur_token = "", dbond_str="", who_str="";
    int n_token = 0;
    for(size_t i = 0; i < memo.size() + 1 && n_token < 4; ++i){
      if((i > 0 && memo[i] == ' ' && memo[i-1] != ' ') || i == memo.size()){
        tokens[n_token] = cur_token;
        ++n_token;
        cur_token = "";
      }
      if(i < memo.size() && memo[i] != ' ')
        cur_token += memo[i];
    }
    if((tokens[0] == "sell" && tokens[2] == "to") || (tokens[0] == "buy" && tokens[2] == "from")){
      dbond_str = tokens[1];
      who_str = tokens[3];
      dbond_id = dbond_id_class(dbond_str);
      who = name(who_str);
      return true;
    }
    else
      return false;
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

} // namespace utility
