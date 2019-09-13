#!/bin/bash

. ../env.sh
. ./common_fc.sh

function authdbond {
	cleos -u $API_URL push action $BANK_ACC authdbond '["'$DBONDS'", "'$bond_name'"]' -p $ADMIN_ACC@active
}

function init_test {
	erase
	initfcdb
	verifyfcdb
	issuefcdb
}

function transfer_to_sell {
	sleep 2
	from="$1"
	to="$2"
	qtty="$3"
	cleos -u $API_URL push action $DBONDS transfer '["'$from'", "'$to'", "'"$qtty"'", "sell"]' -p $from@active
}

title "SELL-BUY TESTS"

title "EMITENT SELLS"
init_test
must_pass "authdbond" authdbond
must_pass "sell" transfer_to_sell $emitent $DBONDS "2.00 $bond_name"

# title "USER BUYS"
# init_test
