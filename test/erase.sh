#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $DBONDS erase "[\"$TESTACC\", \"$bond_name\"]" -p $TESTACC@active
cleos -u $API_URL push action $DBONDS erase "[\"$BUYER\", \"$bond_name\"]" -p $BUYER@active
cleos -u $API_URL push action $DBONDS erase "[\"$DBONDS\", \"$bond_name\"]" -p $DBONDS@active
