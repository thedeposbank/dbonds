#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $collateral_contract transfer "[\"$TESTACC\", \"$DBONDS\", \"$collateral_quantity\", \"put collateral $bond_name\"]"  -p $TESTACC@active
