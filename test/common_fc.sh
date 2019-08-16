#!/bin/bash

now=`date +%s`
now_plus_year=$((now+24*3600*365))
now_plus_360=$((now+24*3600*360))
maturity_time=`date --date=@"$now_plus_year" +%FT%T:000`
dbond_maturity_time=`date --date=@"$now_plus_360" +%FT%T:000`

fiatbond='{"ISIN":"sdf", "name":"sdf", "currency":"sdf", "maturity_time": "'$maturity_time'", "bond_description_webpage":"sdf"}'

bond_name=DBONDA
emitent=$TESTACC
verifier=deposcustody
counterparty=thedeposbank
quantity_to_issue="100.00 $bond_name"
holders_list='["'$emitent'", "'$counterparty'", "'$DBONDS'"]'

payoff_symbol='DUSD'
payoff_contract='thedeposbank'
payoff_amount="10.00"
payoff_quantity="$payoff_amount $payoff_symbol"
payoff_price='{"quantity": "'$payoff_quantity'", "contract": "'$payoff_contract'"}'


bond_spec='{"bond_name": "'$bondname'",
	"emitent": "'$emitent'",
	"quantity_to_issue": "'$quantity_to_issue'",
	"maturity_time": "'$dbond_maturity_time'",
	"payoff_price": '$payoff_price',
	"fungible": true,
	"additional_info": "sdfsdfsdf",
	"collateral_bond": '$fiatbond',
	"verifier": "'$verifier'",
	"counterparty": "'$counterparty'",
	"escrow_contract_link": "https://docs.google.com/document/d/1riKSakwS8p5EVSUA1PL-jCvcjev1kSFfCYR0suBeFkg",
	"annual_interest_rate": 1000,
	"holders_list": '$holders_list'}'

