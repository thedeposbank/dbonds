#!/bin/bash

. ./common.sh

./burn.sh
sleep 5

must_pass "create new dbond" ./create.sh
sleep 5

must_fail "list bond w/o collateral" ./listsale.sh

must_pass "put collateral" ./putcollateral.sh
sleep 5

must_pass "list bond with collateral" ./listsale.sh
sleep 3

echo "bond listed. hit ENTER to continue"
read

must_pass "cancel sale" ./cancelsale.sh
sleep 5

must_fail "cancel sale" ./cancelsale.sh

