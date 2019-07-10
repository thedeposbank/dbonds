#!/bin/bash

. ./common.sh

./erase.sh
sleep 5

must_pass "create new dbond" ./create.sh
sleep 5

save_balances $TESTACC $DBONDS
must_pass "put collateral" ./putcollateral.sh
sleep 5
check_balances $TESTACC "-$collateral_amount" 0 0 0
check_balances $DBONDS  "$collateral_amount"  0 0 0

must_pass "list bond with collateral" ./listsale.sh
sleep 3

# echo "bond listed. hit ENTER to continue"
# read

save_balances $TESTACC $DBONDS $BUYER
must_pass "buy bond" ./buy.sh
sleep 5
check_balances $TESTACC 0 "$buy_amount"  '*' 0
check_balances $DBONDS  0 0 0 0
check_balances $BUYER   0 "-$buy_amount" '*' 1


must_fail "buy bond" ./buy.sh

echo "bond sold. check it on account '$BUYER'. hit ENTER to continue"
read

save_balances $TESTACC $DBONDS
must_pass "pay off bond" ./payoff.sh
sleep 5
check_balances $TESTACC  "$collateral_amount" '*' "-$payoff_amount" 0
check_balances $DBONDS  "-$collateral_amount" '*'  "$payoff_amount" 0

must_fail "repeated pay off bond" ./payoff.sh

must_pass "expire paid off bond" ./expire.sh
sleep 5

must_fail "expire expired bond" ./expire.sh

save_balances $BUYER $DBONDS
must_pass "exchange bond for payoff" ./exchange.sh
sleep 5
check_balances $BUYER  0 '*'  "$payoff_amount" -1
check_balances $DBONDS 0 '*' "-$payoff_amount" 1

must_fail "exchange already exchanged bond" ./exchange.sh

must_pass "burn empty bond" ./burn.sh
