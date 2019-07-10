#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $DBONDS listsale "[\"$TESTACC\", \"$bond_name\"]" -p $TESTACC@active
