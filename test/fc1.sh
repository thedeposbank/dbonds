#!/bin/bash

. ../env.sh
. ./common_fc.sh

function initfcdb {
	sleep 3
	cleos -u $API_URL push action $DBONDS initfcdb "[$bond_spec]" -p $emitent@active
}

function erase {
	sleep 3
	cleos -u $API_URL push action $DBONDS erase '["", "'$bond_name'"]' -p $DBONDS@active
}

function verifyfcdb {
	sleep 3
	cleos -u $API_URL push action $DBONDS verifyfcdb '["'$verifier'", "'$bond_name'"]' -p $verifier@active
}

function issuefcdb {
	sleep 3
	cleos -u $API_URL push action $DBONDS issuefcdb '["'$emitent'", "'$bond_name'"]' -p $emitent@active
}

function confirmfcdb {
	sleep 3
	cleos -u $API_URL push action $DBONDS confirmfcdb '["'$bond_name'"]' -p $counterparty@active
}

#############################

erase
must_pass "initfcdb" initfcdb
must_pass "verifyfcdb" verifyfcdb
must_pass "issuefcdb" issuefcdb
must_pass "confirmfcdb" confirmfcdb
