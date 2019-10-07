#!/bin/bash

#set -x

. ../env.sh
. ./functions.sh

bond_name=DBONDA

collateral_symbol='EOS'
collateral_contract='eosio.token'
collateral_amount="1.0000"
collateral_quantity="$collateral_amount $collateral_symbol"
collateral='{"quantity": "'$collateral_quantity'", "contract": "'$collateral_contract'"}'

buy_symbol='DUSD'
buy_contract='thedeposbank'
buy_amount="2.00"
buy_quantity="$buy_amount $buy_symbol"
buy_price='{"quantity": "'$buy_quantity'", "contract": "'$buy_contract'"}'

payoff_symbol='DUSD'
payoff_contract='thedeposbank'
payoff_amount="10.00"
payoff_quantity="$payoff_amount $payoff_symbol"
payoff_price='{"quantity": "'$payoff_quantity'", "contract": "'$payoff_contract'"}'

fiatbond='{"ISIN":"sdf", "name":"sdf", "currency":"sdf", "country":"sdf", "bond_description_webpage":"sdf", "proof_of_ownership":"sdf"}'
emptyfiatbond='{"ISIN":"", "name":"", "currency":"", "country":"", "bond_description_webpage":"", "proof_of_ownership":""}'

timestamp=`date '+%s'`
timestamp=$((timestamp + 3600*24*7))
maturity_time=`date --date=@$timestamp '+%Y-%m-%dT%H:%M:%S.%N'`
maturity_time=${maturity_time:0:-6}

bondspec='{"bond_name": "'$bond_name'",
	"emitent": "'$TESTACC'",
	"max_supply": "1 '$bond_name'",
	"collateral_type": 0,
	"crypto_collateral": '$collateral',
	"issue_price": '$buy_price',
	"maturity_time": "'$maturity_time'",
	"payoff_price": '$payoff_price',
	"fungible": false,
	"default_case_scenario": 0,
	"early_payoff_policy": 0,
	"collateral_bond": '$emptyfiatbond',
	"additional_info": ""}'
