#!/bin/bash

. ./common.sh

#echo $bondspec

cleos -u $API_URL push action $DBONDS create "[\"$TESTACC\", $bondspec]" -p $TESTACC@active
