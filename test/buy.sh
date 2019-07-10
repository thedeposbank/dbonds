#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $buy_contract transfer "[\"$BUYER\", \"$DBONDS\", \"$buy_quantity\", \"buy bond $bond_name\"]" -p $BUYER@active
