@echo off

REM Written by Patrick Avery
REM Set all of these to be their correct respective directories

set qt5dir=C:\Qt\Qt5.8.0\5.8\msvc2015\

set deps_dir=C:\xtalopt-dependencies-msvc2015

set eigen3_include_dir=%deps_dir%\eigen_3.3.2-1
set libssh_include_dirs=%deps_dir%\libssh_0.7.3-1\include
set libssh_libraries=%deps_dir%\libssh_0.7.3-1\lib\ssh.lib
set qwt_library=%deps_dir%\qwt_6.1.3-2\lib\qwt.lib
set qwt_include_dir=%deps_dir%\qwt_6.1.3-2\include

set build_type=Release

REM This section is only needed if we are building molecular XtalOpt
set enable_molecular=OFF
set rdkit_base_dir=%deps_dir%\rdkit_2017.09.19
set boost_include_dir=%deps_dir%\boost_1.6.2-1\include
set boost_lib_dir=%deps_dir%\boost_1.6.2-1\lib

cmake .. -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=%qt5dir% -DEIGEN3_INCLUDE_DIR=%eigen3_include_dir% -DLIBSSH_INCLUDE_DIRS=%libssh_include_dirs% -DLIBSSH_LIBRARIES=%libssh_libraries% -DQWT_LIBRARY=%qwt_library% -DQWT_INCLUDE_DIR=%qwt_include_dir% -DCMAKE_BUILD_TYPE=%build_type% -DENABLE_MOLECULAR=%enable_molecular% -DRDBASE=%rdkit_base_dir% -DBoost_INCLUDE_DIRS=%boost_include_dir% -DBoost_LIBRARY_DIRS=%boost_lib_dir%
