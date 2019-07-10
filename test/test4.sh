#!/bin/bash

. ./common.sh

./erase.sh
sleep 5

must_pass "create new dbond" ./create.sh
sleep 5

must_pass "put collateral" ./putcollateral.sh
sleep 5

must_pass "list bond with collateral" ./listsale.sh
sleep 3

# echo "bond listed. hit ENTER to continue"
# read

must_pass "buy bond" ./buy.sh
sleep 5

must_fail "buy bond" ./buy.sh

# echo "bond sold. check it on account '$BUYER'. hit ENTER to continue"
# read

must_pass "expire bond" ./expire.sh
sleep 5

must_fail "expire defaulted bond" ./expire.sh

must_pass "exchange bond for collateral" ./exchange.sh
sleep 5

must_fail "exchange already exchanged bond" ./exchange.sh

must_pass "burn empty bond" ./burn.sh
