@echo off

REM Written by Patrick Avery

REM ***NOTE: This script was used in "x64 Native Tools Command Prompt for VS 2017"***
REM ***NOTE: Run this script inside the build folder in the source directory***


REM *********************************************************
REM **** Set all variables to their correct directories  ****
REM *********************************************************

set qt5dir=C:\Qt\Qt5.12.12\5.12.12\msvc2017_64
set qwtdir=C:\Qwt-6.2.0
set insdir=C:\xtalopt_windows
set hasssh=ON
set clissh=ON
set hasdbg=ON

REM *********************************************************
REM **** Configure the build                             ****
REM *********************************************************

cmake .. -G "NMake Makefiles" ^
-DCMAKE_PREFIX_PATH="%qt5dir%;C:\Develop\vcpkg\installed\arm64-windows" ^
-DQWT_LIBRARY=%qwtdir%\lib\qwt.lib ^
-DQWT_INCLUDE_DIR=%qwtdir%\include ^
-DBUILD_INDEPENDENT_PACKAGE=ON ^
-DCMAKE_BUILD_TYPE=Release ^
-DCMAKE_INSTALL_PREFIX=%insdir% ^
-DENABLE_SSH=%hasssh% ^
-DUSE_CLI_SSH=%clissh% ^
-DXTALOPT_DEBUG=%hasdbg% ^
-DCMAKE_TOOLCHAIN_FILE="C:\Develop\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
-DHAVE_EXECINFO_H=OFF

REM *********************************************************
REM **** Compile XtalOpt                                 ****
REM *********************************************************

nmake

REM *********************************************************
REM **** Install XtalOpt                                 ****
REM *********************************************************

nmake install
