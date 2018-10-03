#!/bin/bash

# Just exit if this is clang-format
if [[ "$NAME" == "clang-format" ]]; then
  exit 0
fi

# For GCC only: capture coverage info, filter out system, print debug info,
# and upload to codecov
# Only do codecov if we do not have a tag
if [[ "$NAME" == "gcc-4.8" ]] && [[ -z "$TRAVIS_TAG" ]]; then
  lcov --directory . --capture --output-file coverage.info
  lcov --remove coverage.info '/usr/*' --output-file coverage.info
  lcov --list coverage.info
  bash <(curl -s https://codecov.io/bash) || \
  echo "Codecov did not collect coverage reports"
fi

# If this is a tag, then zip the install directory, md5sum it, and upload it
if [[ -n "$TRAVIS_TAG" ]] && [[ "$TRAVIS_PULL_REQUEST" == "false" ]]; then
  if [[ "$NAME" == "gcc-4.8" ]]; then
    echo "Tag detected for gcc-4.8. Installing, zipping installation, and creating md5sum."
    cd build
    make -j2 install
    cd install
    tar -czvf linux-xtalopt.tgz xtalopt
    md5sum linux-xtalopt.tgz > linux-xtalopt.md5
    # It has been difficult to deploy them from the build directory.
    # Move them to the home directory so we can deploy them from there...
    mv linux-xtalopt.tgz ~/
    mv linux-xtalopt.md5 ~/
  elif [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    echo "Tag detected for osx. Installing, zipping installation, and creating md5sum."
    cd build
    sudo make -j2 install
    sudo chown -R travis ./install
    cd install/xtalopt
    zip -r osx-xtalopt.zip xtalopt.app
    md5 osx-xtalopt.zip > osx-xtalopt.md5
    # It has been difficult to deploy them from the build directory.
    # Move them to the home directory so we can deploy them from there...
    mv osx-xtalopt.zip ~/
    mv osx-xtalopt.md5 ~/
  fi
else
  echo "Tag not detected. Skipping zip and md5sum."
fi
