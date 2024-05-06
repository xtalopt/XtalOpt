INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

set( xtalopt_SRCS
     cliOptions.cpp
     debug.cpp
     xtalopt.cpp
     genetic.cpp
     structures/xtal.cpp
     optimizers/xtaloptoptimizer.cpp
     optimizers/castep.cpp
     optimizers/generic.cpp
     optimizers/gulp.cpp
     optimizers/pwscf.cpp
     optimizers/siesta.cpp
     optimizers/vasp.cpp
     rpc/xtaloptrpc.cpp
     testing/xtalopttest.cpp
     ui/dialog.cpp
     ui/randSpgDialog.cpp
     ui/tab_init.cpp
     ui/tab_edit.cpp
     ui/tab_opt.cpp
     ui/tab_mo.cpp
     ui/tab_progress.cpp
     ui/tab_plot.cpp
     ui/xrd_plot.cpp
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

add_library(xtalopt_static STATIC ${xtalopt_SRCS} ${xtalopt_RCS})
target_link_libraries(xtalopt_static globalsearch XtalComp spglib RandSpg
                      "${QWT_LIBRARIES}")

add_executable(xtalopt WIN32 MACOSX_BUNDLE main.cpp ${xtalopt_RCS})
target_link_libraries(xtalopt xtalopt_static)

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
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/ChangeLog
        DESTINATION "${adjusted_install_prefix}")
install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/schemes
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

  # It appears that we need to install the style as well...
  include(MacroInstallStyle)
  InstallQMacStyle("${CMAKE_INSTALL_PREFIX}/xtalopt.app/Contents/plugins"
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
  # On Windows, make sure that CRT libraries are installed...
  # Also, we need to install the platforms directory in Qt
  if(MSVC)
    set(CMAKE_INSTALL_UCRT_LIBRARIES ON)
    install(DIRECTORY "${QtCore_location}/../plugins/platforms"
      DESTINATION bin
    )
    # If the styles directory exists, install that as well
    if(EXISTS "${QtCore_location}/../plugins/styles")
      install(DIRECTORY "${QtCore_location}/../plugins/styles"
        DESTINATION bin
      )
    endif()
  elseif(UNIX)
    # For unix, install qxcb and its dependencies as well
    include(MacroInstallPlatform)
    InstallQXcbPlatform("${CMAKE_INSTALL_PREFIX}/bin/platforms"
                        "${CMAKE_INSTALL_PREFIX}/lib")
  endif()

  # Let's install required system libraries as well...
  include (InstallRequiredSystemLibraries)

  # Finally, install dependencies that aren't system libraries...
  include(MacroInstallDependencies)
  InstallDependencies("${ExeLocation}" "${LibLocation}" "${DEP_SEARCH_DIRS}")
endif()
