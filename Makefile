ifndef CPPFLAGS
override CPPFLAGS = -DBITCOIN_TESTNET=true -DDEBUG
endif

all: dbonds.wasm

dbonds.wasm: src/dbonds.cpp include/dbonds.hpp include/dbond.hpp include/utility.hpp
	eosio-cpp src/dbonds.cpp $(CPPFLAGS) -o dbonds.wasm -I./include -abigen -contract dbonds

install: dbonds.wasm
	cleos -u $(API_URL) set contract $(DBONDS) .

clean:
	rm -f *.abi *.wasm

test: install
	. ./env.sh ; cd test ; ./fc1.sh && ./fc2.sh && ./fc3.sh

