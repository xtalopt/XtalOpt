#!/bin/bash

# Before script
# For Linux, we have to load the travis-ci display to run the tests even
# though we don't actually need the display...
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_PREFIX_PATH=/usr/local/opt/qt/"
  CMAKE_FLAGS="$CMAKE_FLAGS -DQWT_INCLUDE_DIR=/usr/local/opt/qwt/lib/qwt.framework/Headers"
else
  export DISPLAY=:99.0
  sh -e /etc/init.d/xvfb start
  sleep 3
  CMAKE_FLAGS="$CMAKE_FLAGS -DENABLE_MOLECULAR=ON"
fi

CMAKE_FLAGS="$CMAKE_FLAGS -DBUILD_INDEPENDENT_PACKAGE=ON"
CMAKE_FLAGS="$CMAKE_FLAGS -DCMAKE_INSTALL_PREFIX=install/xtalopt"
CMAKE_FLAGS="$CMAKE_FLAGS -DBUILD_TESTS=ON"
mkdir build
cd build
mkdir install
cmake ${CMAKE_FLAGS} ..

# Script
make -j2
ctest --output-on-failure
