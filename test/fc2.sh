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
	state=`echo "$fcdb_states" | grep -w "$1" | egrep -o '[0-9]+'`
	cleos -u $API_URL push action $DBONDS setstate '["'$bond_name'", '$state']' -p $emitent@active
}

function transfer_dbond {
	from=$1
	to=$2
	cleos -u $API_URL push action $DBONDS transfer '["'$from'", "'$to'", "'$quantity_to_issue'", "retire '$bond_name'"]' -p $from@active
}

title "RETIREMENT TESTS"

title "EMITENT SENDS ALL DBONDS"
init_test
setstate EXPIRED_PAID_OFF
must_pass transfer_dbond $emitent $DBONDS


title "EMITENT SENDS DUSD"

title "LIQUIDATION AGENT SENDS DUSD"
