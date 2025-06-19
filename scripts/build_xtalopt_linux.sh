#!/bin/bash

# Script to build XtalOpt on a linux system, given that all
#   requirements already installed.
# It is assumed that this is run in "xtalopt-source"/build

# *********************************************************
# **** Set all variables to their correct directories  ****
# *********************************************************

insdir=$PWD/../xtalopt_linux
instal=ON
hasssh=ON
clissh=ON
hasdbg=OFF

# *********************************************************
# **** Configure the build                             ****
# *********************************************************

cmake -DBUILD_INDEPENDENT_PACKAGE=$instal \
      -DINSTALL_DEPENDENCIES=$instal \
      -DCMAKE_INSTALL_PREFIX=$insdir \
      -DENABLE_SSH=$hasssh \
      -DUSE_CLI_SSH=$clissh \
      -DXTALOPT_DEBUG=$hasdbg \
      ..

# *********************************************************
# **** Compile XtalOpt                                 ****
# *********************************************************

make -j3
