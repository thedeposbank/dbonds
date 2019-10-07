#!/bin/bash

. ../env.sh
. ./common_fc.sh

function init_test {
	erase
	initfcdb
	verifyfcdb
	issuefcdb
	confirmfcdb
}

function setstate {
	sleep 2
	state=`echo "$fcdb_states" | grep -w "$1" | egrep -o '[0-9]+'`
	cleos -u $API_URL push action $DBONDS setstate '["'$bond_name'", '$state']' -p $emitent@active
}

function transfer_dbond {
	sleep 2
	from="$1"
	to="$2"
	qtty="$3"
	cleos -u $API_URL push action $DBONDS transfer '["'$from'", "'$to'", "'"$qtty"'", "retire '$bond_name'"]' -p $from@active
}

function transfer_payoff {
	sleep 2
	from="$1"
	to="$2"
	qtty="$3"
	cleos -u $API_URL push action $payoff_contract transfer '["'$from'", "'$to'", "'"$qtty"'", "retire '$bond_name'"]' -p $from@active
}

title "RETIREMENT TESTS"

title "EMITENT SENDS ALL DBONDS"
init_test
must_fail "not expired" transfer_dbond $emitent $DBONDS "$quantity_to_issue"
setstate EXPIRED_PAID_OFF
must_fail "expired, not all sent" transfer_dbond $emitent $DBONDS "1.00 $bond_name"
must_fail "expired, all sent" transfer_dbond $emitent $DBONDS "$quantity_to_issue"

title "EMITENT SENDS DUSD"
init_test
must_fail "not DUSD" transfer_payoff $emitent $DBONDS "1.00000000 DPS"
must_pass "transfer part of DBOND to bank" transfer_dbond $emitent $counterparty "3.00 $bond_name"
must_pass "DUSD" transfer_payoff $emitent $DBONDS "55.00 DUSD"

title "LIQUIDATION AGENT SENDS DUSD"
init_test
setstate EXPIRED_TECH_DEFAULTED
must_fail "not agent" transfer_payoff $emitent $DBONDS "2.00 DUSD"
must_pass "agent" transfer_payoff $liquidation_agent $DBONDS "3.00 DUSD"
