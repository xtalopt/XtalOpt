INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

find_package(Qwt REQUIRED)
include_directories(${QWT_INCLUDE_DIRS})

set( xtalopt_SRCS
     cliOptions.cpp
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

set( xtalopt_RCS xtalopt.qrc )

# This is so the executable will have an icon
if(MSVC)
  set (xtalopt_RCS ${xtalopt_RCS} xtalopt.rc)
endif(MSVC)

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
  set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib${LIB_SUFFIX}")
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

add_executable(xtalopt WIN32 MACOSX_BUNDLE main.cpp
               ${xtalopt_SRCS} ${xtalopt_RCS})

target_link_libraries(xtalopt globalsearch XtalComp spglib RandSpg
                      "${QWT_LIBRARIES}")

# Install instructions
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(UNIX)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}" CACHE PATH
        "default install path" FORCE)
  elseif(WIN32)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/XtalOpt" CACHE PATH
        "default install path" FORCE)
  endif()
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

install(TARGETS xtalopt
  BUNDLE DESTINATION .
  RUNTIME DESTINATION bin
)

set(adjusted_install_prefix ".")
if(APPLE)
  set(adjusted_install_prefix "./xtalopt.app/Contents")
endif(APPLE)

install(FILES
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/COPYING
        DESTINATION "${adjusted_install_prefix}")
install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/ChangeLogs
        ${CMAKE_SOURCE_DIR}/samples
        DESTINATION "${adjusted_install_prefix}")

# Get all dependencies and perform the "bundle" for Mac OSX
if(APPLE)
  # Install the icon to the resources directory
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/xtalopt-logo.icns"
          DESTINATION "${adjusted_install_prefix}/Resources")
  # Set the icon file in the plist
  set(MACOSX_BUNDLE_ICON_FILE "xtalopt-logo.icns")

  # I'm not sure why cmake doesn't bundle cocoa for us, but since
  # it doesn't, we need to install it ourselves...
  include(MacroInstallCocoa)
  InstallCocoa("${CMAKE_INSTALL_PREFIX}/xtalopt.app/Contents/plugins"
               QT_PLUGINS)

  # We need to write a qt.conf file that points to the plugins
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
       "[Paths]\nPlugins = plugins\n")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
          DESTINATION "${adjusted_install_prefix}/Resources")

  # Time to fix up the application bundle!
  set(APPS "\${CMAKE_INSTALL_PREFIX}/xtalopt.app")
  install(CODE "
     include(BundleUtilities)
     fixup_bundle(\"${APPS}\"   \"${QT_PLUGINS}\"   \"\")
     " COMPONENT Runtime)
endif()

set(ExeLocation "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xtalopt")
set(LibLocation "lib")
if(WIN32)
  set(ExeLocation "${ExeLocation}.exe")
  set(LibLocation "bin")
endif(WIN32)

# If XtalOpt hasn't been built yet and INSTALL_DEPENDENCIES has
# been set, print a warning to the user. It isn't really a problem,
# but cmake will be run again afterwards so that we can find its
# dependencies. Just let the user know this.
if(INSTALL_DEPENDENCIES AND NOT EXISTS "${ExeLocation}"
   AND NOT APPLE)
  message("-- warning: INSTALL_DEPENDENCIES was set before xtalopt was built")
  message("-- warning: CMake will be automatically called again after xtalopt is built in order to install the dependencies")
  add_custom_command(TARGET xtalopt
                     COMMAND "${CMAKE_COMMAND}" -E cmake_echo_color
                     "-- Re-running cmake to locate xtalopt dependencies"
                     POST_BUILD)
  add_custom_command(TARGET xtalopt
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                     COMMAND "${CMAKE_COMMAND}" "${CMAKE_SOURCE_DIR}" POST_BUILD)
endif()

if(INSTALL_DEPENDENCIES AND EXISTS "${ExeLocation}" AND NOT APPLE)
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

  include(MacroInstallDependencies)
  InstallDependencies("${ExeLocation}" "${LibLocation}" "${DEP_SEARCH_DIRS}")
endif()
