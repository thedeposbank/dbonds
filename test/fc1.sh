#!/bin/bash

. ../env.sh
. ./common_fc.sh

title "ISSUANCE TESTS"

title "'RIGHT' SCENARIO"
erase
must_pass "initfcdb" initfcdb
must_pass "verifyfcdb" verifyfcdb
must_pass "issuefcdb" issuefcdb
must_pass "confirmfcdb" confirmfcdb
current_price=`get_extended_asset current_price`
initial_price=`get_extended_asset initial_price`
must_pass "check current price" [ "$current_price" = "9.11 DUSD@thedeposbank" ]
must_pass "check initial price" [ "$initial_price" = "9.11 DUSD@thedeposbank" ]

title "ISSUANCE BEFORE VERIFICATION"
erase
must_pass "initfcdb" initfcdb
must_fail "issuefcdb" issuefcdb

title "CONFIRMATION BEFORE VERIFICATION"
erase
must_pass "initfcdb" initfcdb
must_fail "confirmfcdb" confirmfcdb

title "CONFIRMATION BEFORE ISSUANCE"
erase
must_pass "initfcdb" initfcdb
must_pass "verifyfcdb" verifyfcdb
must_pass "confirmfcdb" confirmfcdb
must_pass "issuefcdb" issuefcdb

title "UPDATE AFTER CREATION"
erase
must_pass "initfcdb" initfcdb
must_pass "initfcdb" initfcdb "$bond_spec2"

function verifyfcdb_unauth {
	sleep 3
	cleos -u $API_URL push action $DBONDS verifyfcdb '["'$verifier'", "'$bond_name'"]' -p $TESTACC@active
}

function confirmfcdb_unauth {
	sleep 3
	cleos -u $API_URL push action $DBONDS confirmfcdb '["'$bond_name'"]' -p $TESTACC@active
}

function issuefcdb_unauth {
	sleep 3
	cleos -u $API_URL push action $DBONDS issuefcdb '["'$emitent'", "'$bond_name'"]' -p $BUYER@active
}

title "UNATHORIZED VERIFICATION"
erase
must_pass "initfcdb" initfcdb
must_fail "unauthorized verifyfcdb" verifyfcdb_unauth

title "UNATHORIZED CONFIRMATION"
erase
must_pass "initfcdb" initfcdb
must_pass "verifyfcdb" verifyfcdb
must_fail "unauthorized confirmfcdb" confirmfcdb_unauth

title "UNATHORIZED ISSUANCE"
erase
must_pass "initfcdb" initfcdb
must_pass "verifyfcdb" verifyfcdb
must_fail "unauthorized issuefcdb" issuefcdb_unauth

erase
