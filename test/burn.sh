#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $DBONDS burn "[\"$TESTACC\", \"$bond_name\"]" -p $TESTACC@active
