#!/bin/bash

# Script to build XtalOpt on a linux system, given that all
#   requirements already installed.
# It is assumed that this is run in "xtalopt-source"/build

# *********************************************************
# **** Set all variables to their correct directories  ****
# *********************************************************

insdir=$PWD/../xtalopt_macos
qt5dir=/opt/homebrew/opt/qt@5
qwtdir=/opt/homebrew/opt/qwt-qt5
libssh=/opt/homebrew/opt/libssh
buildt=Release
instal=ON
hasssh=ON
clissh=ON
hasdbg=OFF

# *********************************************************
# **** Configure the build                             ****
# *********************************************************

cmake -DCMAKE_PREFIX_PATH=$qt5dir/lib/cmake/Qt5 \
      -DQWT_LIBRARY=$qwtdir/lib/qwt.framework/qwt \
      -DQWT_INCLUDE_DIR=$qwtdir/lib/qwt.framework/Headers \
      -DBUILD_INDEPENDENT_PACKAGE=$instal \
      -DINSTALL_DEPENDENCIES=$instal \
      -DCMAKE_INSTALL_PREFIX=$insdir \
      -DLIBSSH_INCLUDE_DIRS=$libssh/include \
      -DLIBSSH_LIBRARIES=$libssh/lib/libssh.dylib \
      -DENABLE_SSH=$hasssh \
      -DUSE_CLI_SSH=$clissh \
      -DXTALOPT_DEBUG=$hasdbg \
      ..

# *********************************************************
# **** Compile XtalOpt                                 ****
# *********************************************************

make -j3

exit
