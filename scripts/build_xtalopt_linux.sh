#!/bin/bash
set -e

# Script to build the XtalOpt GUI on a linux system, given that all
#   requirements are already installed (Qt, Qwt, libssh).
# It is assumed that this is run in "xtalopt-source"/build

# *********************************************************
# **** Set all variables to their correct directories  ****
# *********************************************************

insdir=$PWD/../xtalopt_linux
buildt=Release
instal=ON
hasssh=ON

# *********************************************************
# **** Configure the build                             ****
# *********************************************************

cmake -DBUILD_XTALOPT_GUI=ON \
      -DBUILD_INDEPENDENT_PACKAGE=$instal \
      -DCMAKE_BUILD_TYPE=$buildt \
      -DCMAKE_INSTALL_PREFIX=$insdir \
      -DBUILD_WITH_LIBSSH=$hasssh \
      ..

# *********************************************************
# **** Compile XtalOpt                                 ****
# *********************************************************

make -j3
