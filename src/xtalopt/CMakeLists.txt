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

# Set up RPATH if we need to. This is necessary for a stand-alone package.
if(UNIX AND ENABLE_RPATH)
  if (APPLE)
    # No instructions yet...
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib${LIB_SUFFIX}")
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  endif()
endif()

# Add resources
qt5_wrap_ui(plugin_UIS_H ${xtalopt_UIS})
qt5_add_resources(plugin_RC_SRCS ${xtalopt_RCS})

if(WIN32)
  add_executable(xtalopt WIN32 main.cpp ${xtalopt_SRCS} ${plugin_UIS_H}
                 ${plugin_RC_SRCS} ${moc_files})
else()
  add_executable(xtalopt main.cpp ${xtalopt_SRCS} ${plugin_UIS_H}
                 ${plugin_RC_SRCS} ${moc_files})
endif()

target_link_libraries(xtalopt globalsearch XtalComp spglib RandSpg
                      "${QWT_LIBRARIES}")

# Install instructions
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH
        "default install path" FORCE)
  elseif(WIN32)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/XtalOpt" CACHE PATH
        "default install path" FORCE)
  endif()
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if(NOT APPLE)
  install(TARGETS xtalopt
    RUNTIME DESTINATION bin
  )
  install(FILES
          ${CMAKE_SOURCE_DIR}/README.md
          ${CMAKE_SOURCE_DIR}/COPYING
          DESTINATION ${CMAKE_INSTALL_PREFIX})
  install(DIRECTORY
          ${CMAKE_SOURCE_DIR}/ChangeLogs
          ${CMAKE_SOURCE_DIR}/samples
          DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

if(INSTALL_DEPENDENCIES)
  # We need to tell Windows (and UNIX if there are any dependencies
  # that are not in the path) exactly where to search for dynamically linked
  # libraries
  # ssh.dll should be in dir(${LIBSSH_LIBRARIES})\..\bin
  get_filename_component(LIBSSH_DLL_DIR "${LIBSSH_LIBRARIES}" DIRECTORY)
  set(DEP_SEARCH_DIRS "${DEP_SEARCH_DIRS}" "${LIBSSH_DLL_DIR}/../bin")

  # qwt.dll should be in the same dir as ${QWT_LIBRARIES}
  get_filename_component(QWT_DLL_DIR "${QWT_LIBRARIES}" DIRECTORY)
  set(DEP_SEARCH_DIRS "${DEP_SEARCH_DIRS}" "${QWT_DLL_DIR}")

  # All of the Qt dependencies will hopefully be together in the bin of the
  # root directory. We will need to change this part in the future if they
  # are not.
  get_target_property(QtCore_location Qt5::Core LOCATION)
  get_filename_component(QtCore_location "${QtCore_location}" DIRECTORY)
  set(DEP_SEARCH_DIRS "${DEP_SEARCH_DIRS}" "${QtCore_location}/../bin")

  set(ExeLocation "${CMAKE_INSTALL_PREFIX}/bin/xtalopt")
  set(LibLocation "${CMAKE_INSTALL_PREFIX}/lib")
  if(WIN32)
    set(ExeLocation "${ExeLocation}.exe")
    set(LibLocation "${CMAKE_INSTALL_PREFIX}/bin")
  endif(WIN32)

  install(CODE
          "
          include(${CMAKE_SOURCE_DIR}/cmake/modules/InstallDependencies.cmake)
          InstallDependencies(\"${ExeLocation}\"
                              \"${LibLocation}\"
                              \"${DEP_SEARCH_DIRS}\")
          ")
endif(INSTALL_DEPENDENCIES)
