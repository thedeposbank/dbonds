#!/bin/bash

. ./common.sh

./burn.sh
sleep 5

must_fail "burn non-existent dbond" ./burn.sh

must_pass "create new dbond" ./create.sh
sleep 5

must_fail "create existing dbond" ./create.sh

must_pass "burn existing dbond" ./burn.sh
sleep 5

must_fail "put collateral" ./putcollateral.sh

./create.sh
sleep 5

must_pass "put collateral" ./putcollateral.sh
sleep 5

must_fail "put collateral" ./putcollateral.sh

must_pass "burn collateralized dbond" ./burn.sh
