#!/bin/bash

function must_pass() {
	testname="$1"
	shift 1
	"$@"
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
	"$@"
	if [[ $? = 0 ]] ; then
		echo -e "\e[31m$testname ERROR, wrongly passed: $@\e[0m"
		exit 1
	else
		echo -e "\e[32m$testname OK, expectedly failed: $@\e[0m"
	fi
}

function title() {
	title="# $1 #"
	hashes=`echo "$title" | tr '[\040-\377]' '[#*]'`
	echo
	echo -e "\e[32m$hashes\e[0m"
	echo -e "\e[32m$title\e[0m"
	echo -e "\e[32m$hashes\e[0m"
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
