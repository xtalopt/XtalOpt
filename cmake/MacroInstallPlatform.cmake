# Written by Patrick Avery - 2018
# This is a macro for installing platform dependencies during a package
# build.

# This will install a .so file as well as its dependencies
macro(InstallPlatform _file _destination _dep_destination)
  include(MacroInstallDependencies)
  InstallDependencies("${_file}" "${_dep_destination}" "")
  install(FILES "${_file}"
    DESTINATION "${_destination}"
  )
endmacro()

# Find and install QXcb and its dependencies for Linux.
# The _destination is the destination for the qxcb library.
# The _dep_destination is the destination for the qxcb dependencies.
macro(InstallQXcbPlatform _destination _dep_destination)

  if(TARGET "Qt5::QXcbIntegrationPlugin")
    get_target_property(_qxcb_loc Qt5::QXcbIntegrationPlugin LOCATION)
  endif()

  if(NOT EXISTS "${_qxcb_loc}")
    # If it was not found, try to find it using common paths
    # Might be able to find it relative to qmake
    get_property(_qmake_location TARGET ${Qt5Core_QMAKE_EXECUTABLE}
                 PROPERTY IMPORT_LOCATION)
    get_filename_component(_qmake_path ${_qmake_location} DIRECTORY)
    find_file(_qxcb_loc "libqxcb.so"
              HINTS "${_qmake_path}/../plugins/platforms/"
              PATHS "/usr/lib/x86_64-linux-gnu/qt/plugins/platforms"
                    "/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms"
                    "/usr/lib/qt/plugins/platforms"
                    "/usr/lib/qt5/plugins/platforms")
    # Feel free to append paths to the above if more paths are needed.
  endif()

  if(EXISTS "${_qxcb_loc}")
    InstallPlatform("${_qxcb_loc}" "${_destination}" "${_dep_destination}")
  else()
    message(FATAL_ERROR "Unable to find QXcb library")
  endif()
endmacro()
