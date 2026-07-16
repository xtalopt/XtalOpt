#!/bin/bash
set -e

# Script to build the XtalOpt GUI on a macOS system, given that all
#   requirements are already installed (e.g. via homebrew: qt, qwt, libssh).
# It is assumed that this is run in "xtalopt-source"/build

# *********************************************************
# **** Set all variables to their correct directories  ****
# *********************************************************

insdir=$PWD/../xtalopt_macos
qtdir=/opt/homebrew/opt/qt
qwtdir=/opt/homebrew/opt/qwt
libssh=/opt/homebrew/opt/libssh
buildt=Release
instal=ON
hasssh=ON

# *********************************************************
# **** Configure the build                             ****
# *********************************************************

cmake -DCMAKE_PREFIX_PATH=$qtdir \
      -DQWT_LIBRARY=$qwtdir/lib/qwt.framework/qwt \
      -DQWT_INCLUDE_DIR=$qwtdir/lib/qwt.framework/Headers \
      -DBUILD_XTALOPT_GUI=ON \
      -DBUILD_INDEPENDENT_PACKAGE=$instal \
      -DCMAKE_BUILD_TYPE=$buildt \
      -DCMAKE_INSTALL_PREFIX=$insdir \
      -DLIBSSH_INCLUDE_DIRS=$libssh/include \
      -DLIBSSH_LIBRARIES=$libssh/lib/libssh.dylib \
      -DBUILD_WITH_LIBSSH=$hasssh \
      ..

# *********************************************************
# **** Compile XtalOpt                                 ****
# *********************************************************

make -j3

exit
