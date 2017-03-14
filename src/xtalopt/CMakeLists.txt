INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

find_package(Qwt REQUIRED)
include_directories(${QWT_INCLUDE_DIRS})

set( xtalopt_SRCS
     debug.cpp
     xtalopt.cpp
     genetic.cpp
     structures/xtal.cpp
     optimizers/xtaloptoptimizer.cpp
     optimizers/vasp.cpp
     optimizers/gulp.cpp
     optimizers/pwscf.cpp
     optimizers/castep.cpp
     optimizers/siesta.cpp
     testing/xtalopttest.cpp
     ui/dialog.cpp
     ui/randSpgDialog.cpp
     ui/tab_init.cpp
     ui/tab_edit.cpp
     ui/tab_opt.cpp
     ui/tab_progress.cpp
     ui/tab_plot.cpp
     ui/xtalopt_plot.cpp
     ui/tab_log.cpp)

set( xtalopt_UIS
     ui/dialog.ui
     ui/tab_init.ui
     ui/tab_opt.ui
     ui/tab_progress.ui
     ui/tab_plot.ui
     ui/tab_log.ui
     ui/randSpgDialog.ui)

set( xtalopt_RCS xtalopt.qrc )

# Get rid of a few annoying warnings. In the future, we can perhaps
# clean up the warnings rather than ignore them... except for the
# Clang one. That one is caused by Eigen.
if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif(UNIX)
# This one shows up on some clang compilers with Eigen3
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
endif()
if(MSVC)
  # Ignore sign comparison warning
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018")
endif(MSVC)

# Add resources
qt5_wrap_ui(plugin_UIS_H ${xtalopt_UIS})
qt5_add_resources(plugin_RC_SRCS ${xtalopt_RCS})
add_executable(xtalopt main.cpp ${xtalopt_SRCS} ${plugin_UIS_H}
                       ${plugin_RC_SRCS} ${moc_files})

target_link_libraries(xtalopt globalsearch XtalComp spglib RandSpg
                      "${QWT_LIBRARIES}")

# Install instructions
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH
        "default install path" FORCE)
  endif()
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if(UNIX AND NOT APPLE)
  install(TARGETS xtalopt
    RUNTIME DESTINATION bin
  )
endif()
