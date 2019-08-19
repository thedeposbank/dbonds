#!/bin/bash

. ../env.sh
. ./common_fc.sh


# "right" scenario
erase
must_pass "initfcdb" initfcdb
must_pass "verifyfcdb" verifyfcdb
must_pass "issuefcdb" issuefcdb
must_pass "confirmfcdb" confirmfcdb

# issuance before verification
erase
must_pass "initfcdb" initfcdb
must_fail "issuefcdb" issuefcdb

# confirmation before verification
erase
must_pass "initfcdb" initfcdb
must_fail "confirmfcdb" confirmfcdb
