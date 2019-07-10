#!/bin/bash

. ./common.sh

cleos -u $API_URL push action $DBONDS expire "[\"$bond_name\"]" -p $TESTACC@active
