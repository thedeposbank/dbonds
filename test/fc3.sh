#!/bin/bash

. ../env.sh
. ./common_fc.sh

function authdbond {
	cleos -u $API_URL push action $BANK_ACC authdbond '["'$DBONDS'", "'$bond_name'"]' -p $ADMIN_ACC@active
}

function authdbond_unauth {
	cleos -u $API_URL push action $BANK_ACC authdbond '["'$DBONDS'", "'$bond_name'"]' -p $TESTACC@active
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
	cleos -u $API_URL push action $DBONDS transfer '["'$from'", "'$to'", "'"$qtty"'", "sell '$bond_name' to '$counterparty'"]' -p $from@active
}

function transfer_to_buy {
	sleep 2
	from="$1"
	to="$2"
	qtty="$3"
	cleos -u $API_URL push action $BANK_ACC transfer '["'$from'", "'$to'", "'"$qtty"'", "buy '$bond_name' from '$counterparty'"]' -p $from@active
}

title "SELL-BUY TESTS"

title "EMITENT SELLS"
init_test
must_fail "authdbond" authdbond_unauth
must_pass "authdbond" authdbond
must_fail "sell wrong tokens" transfer_to_sell $emitent $DBONDS "1.23 SOMETKN"
must_pass "sell" transfer_to_sell $emitent $DBONDS "2.00 $bond_name"

title "USER BUYS"
must_fail "buy wrong tokens" transfer_to_buy $emitent $DBONDS "1.23 SOMETKN"
must_pass "buy" transfer_to_buy $emitent $DBONDS "17.00 DUSD"

title "USER BUYS MORE THAN BANK HAS"
init_test
must_fail "authdbond" authdbond_unauth
must_pass "authdbond" authdbond
must_pass "sell" transfer_to_sell $emitent $DBONDS "2.00 $bond_name"
must_fail "buy more than available" transfer_to_buy $emitent $DBONDS "25.00 DUSD"
