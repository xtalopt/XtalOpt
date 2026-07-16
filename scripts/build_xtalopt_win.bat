@echo off

REM Written by Patrick Avery

REM ***NOTE: Run this script in the "x64 Native Tools Command Prompt" of a
REM          recent Visual Studio, inside the build folder in the source
REM          directory. Adjust the Qt/Qwt/vcpkg paths to the installed kits;
REM          the vcpkg triplet must match the Qt architecture (x64 below).***

REM *********************************************************
REM **** Set all variables to their correct directories  ****
REM *********************************************************

set qtdir=C:\Qt\6.5.3\msvc2019_64
set qwtdir=C:\Qwt-6.3.0
set insdir=C:\xtalopt_windows
set hasssh=ON

REM *********************************************************
REM **** Configure the build                             ****
REM *********************************************************

cmake .. -G "NMake Makefiles" ^
-DCMAKE_PREFIX_PATH="%qtdir%;C:\Develop\vcpkg\installed\x64-windows" ^
-DQWT_LIBRARY=%qwtdir%\lib\qwt.lib ^
-DQWT_INCLUDE_DIR=%qwtdir%\include ^
-DBUILD_XTALOPT_GUI=ON ^
-DBUILD_INDEPENDENT_PACKAGE=ON ^
-DCMAKE_BUILD_TYPE=Release ^
-DCMAKE_INSTALL_PREFIX=%insdir% ^
-DBUILD_WITH_LIBSSH=%hasssh% ^
-DCMAKE_TOOLCHAIN_FILE="C:\Develop\vcpkg\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 exit /b 1

REM *********************************************************
REM **** Compile XtalOpt                                 ****
REM *********************************************************

nmake
if errorlevel 1 exit /b 1

REM *********************************************************
REM **** Install XtalOpt                                 ****
REM *********************************************************

nmake install
