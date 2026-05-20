# abstracton
Regular Model Checking using Regular Abstraction Frameworks

## Building
Run `cmake .` followed by `make`.

### Dependencies
This project depends on:
- [mata](https://github.com/VeriFIT/mata)
    - Currently, on this branch (`dodo-benchmarks-lazy-tree`) use [this](https://github.com/tmokenc/mata/tree/lazy-nft-generic-arity) fork on branch `lazy-nft-generic-arity` instead!
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) for running the benchmarks
Please refer to their build instructions first.

## Running the tests
After Building, you can run the tests by executing `make test`. Tests can be rebuilt by `make tests`.
