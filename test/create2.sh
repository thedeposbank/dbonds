#!/bin/bash

. ./common.sh

bondspec='{"bond_name": "'$bond_name'",
	"emitent": "'$TESTACC'",
	"max_supply": "1 '$bond_name'",
	"crypto_collateral": {"quantity": "0.000 EOS", "contract": "eosio.token"},
	"collateral_type": 2,
	"issue_price": '$buy_price',
	"maturity_time": "'$maturity_time'",
	"payoff_price": '$payoff_price',
	"fungible": false,
	"default_case_scenario": 0,
	"early_payoff_policy": 0,
	"collateral_bond": '$emptyfiatbond',
	"additional_info": ""}'


cleos -u $API_URL push action $DBONDS create "[\"$TESTACC\", $bondspec]" -p $TESTACC@active
