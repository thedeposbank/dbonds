#!/bin/bash

#set -x

. ../env.sh

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

function must_pass() {
	testname="$1"
	shift 1
	$@
	if [[ $? = 0 ]] ; then
		echo -e "\e[32m$testname OK, expectedly passed: $@\e[0m"
	else
		echo -e "\e[31m$testname ERROR, wrongly failed: $@\e[0m"
		exit 2
	fi
}

function must_fail() {
	testname="$1"
	shift 1
	$@
	if [[ $? = 0 ]] ; then
		echo -e "\e[31m$testname ERROR, wrongly passed: $@\e[0m"
		exit 1
	else
		echo -e "\e[32m$testname OK, expectedly failed: $@\e[0m"
	fi
}

# print balances of collateral, nomination, payoff, dbond tokens of given account
function get_balances {
	account=$1
	collateral_balance=`cleos -u $API_URL get currency balance $collateral_contract $account $collateral_symbol | cut -f 1 -d ' '`
	collateral_balance=${collateral_balance:-0}
	nomination_balance=`cleos -u $API_URL get currency balance $buy_contract $account $buy_symbol | cut -f 1 -d ' '`
	nomination_balance=${nomination_balance:-0}
	payoff_balance=`cleos -u $API_URL get currency balance $payoff_contract $account $payoff_symbol | cut -f 1 -d ' '`
	payoff_balance=${payoff_balance:-0}
	dbonds_balance=`cleos -u $API_URL get currency balance $DBONDS $account $bond_name | cut -f 1 -d ' '`
	dbonds_balance=${dbonds_balance:-0}
	echo -e "$collateral_balance $nomination_balance $payoff_balance $dbonds_balance"
}

function sub {
	echo "$1 - $2" | bc -l
}

# read two lines of balances, substract first line's values from second one's
function diff_balances {
	read a1 a2 a3 a4
	read b1 b2 b3 b4
	echo `sub $b1 $a1` `sub $b2 $a2` `sub $b3 $a3` `sub $b4 a4`
}

declare -A balances

# save balances for given accounts to named array cells
function save_balances {
	for account in $@
	do
		balances[$account]=`get_balances $account`
	done
}

# check balances diffs for given account
function check_balances {
	account=$1
	shift
	before=`echo ${balances[$account]}`
	current=`get_balances $account`
	differ="$@"
	for i in 1 2 3 4
	do
		b=`echo "$before" | cut -f $i -d ' '`
		c=`echo "$current" | cut -f $i -d ' '`
		d=$1
		shift
		if [[ "$d" = '*' ]] ; then
			continue
		fi
		result=`echo "$c-$b - ($d)" | bc -l`
		if [[ "$result" != 0 ]] ; then
			echo -e "\e[31mcheck balances ERROR: '$before', '$current', '$differ'\e[0m"
			exit 3
		else
			echo -e "\e[32mcheck balances OK\e[0m"
		fi
	done
}
