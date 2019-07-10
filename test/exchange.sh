#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $DBONDS transfer "[\"$BUYER\", \"$DBONDS\", \"1 $bond_name\", \"exchange\"]" -p $BUYER@active
