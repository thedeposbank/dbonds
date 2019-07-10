#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $payoff_contract transfer "[\"$TESTACC\", \"$DBONDS\", \"$payoff_quantity\", \"pay off bond $bond_name\"]" -p $TESTACC@active
