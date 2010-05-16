#!/bin/bash

# Check for a config file
if [ ! -f tests/config ]
then
    echo "XTALOPT_ROOT=/usr/src/xtalopt" >> tests/config;
    echo "XTALOPT_SO=\$XTALOPT_ROOT/build/xtalopt.so" >> tests/config;
    echo "Created tests/config. Check the values within it before continuing."
    exit 1;
fi

FAIL=""

# Run tests
cd tests
. config

source checkForUndefinedSymbols.sh

if [ ! -z "$FAIL" ]
then
    echo
    echo "One or more tests failed:"
    echo -e "$FAIL"
    echo
    exit 1;
else
    echo "All tests ran successfully."
    exit 0;
fi
