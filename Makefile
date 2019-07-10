all: dbonds.wasm

dbonds.wasm: src/dbonds.cpp include/dbonds.hpp include/dbond.hpp include/utility.hpp
	eosio-cpp src/dbonds.cpp $(CPPFLAGS) -o dbonds.wasm -I./include -abigen -contract dbonds

install: dbonds.wasm
	. ./env.sh
	cleos -u $(API_URL) set contract $(DBONDS) .

clean:
	rm -f *.abi *.wasm
