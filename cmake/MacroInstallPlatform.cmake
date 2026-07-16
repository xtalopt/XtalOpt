# Written by Patrick Avery - 2018
# This is a macro for installing platform dependencies during a package
# build.

# Install a .so file and its dependencies. Keep the Qt plugin rpath unchanged.
macro(InstallPlatform _file _destination _dep_destination)
  include(MacroInstallDependencies)
  InstallDependencies("${_file}" "${_dep_destination}" "${DEP_SEARCH_DIRS}")
  install(FILES "${_file}"
    DESTINATION "${_destination}"
  )
endmacro()

# Find and install QXcb and its dependencies for Linux.
# The _destination is the destination for the qxcb library.
# The _dep_destination is the destination for the qxcb dependencies.
macro(InstallQXcbPlatform _destination _dep_destination)

  if(TARGET "Qt${QT_VERSION_MAJOR}::QXcbIntegrationPlugin")
    get_target_property(_qxcb_loc Qt${QT_VERSION_MAJOR}::QXcbIntegrationPlugin IMPORTED_LOCATION_RELEASE)
    if(NOT _qxcb_loc)
      get_target_property(_qxcb_loc Qt${QT_VERSION_MAJOR}::QXcbIntegrationPlugin IMPORTED_LOCATION)
    endif()
    if(NOT _qxcb_loc)
      get_target_property(_qxcb_loc Qt${QT_VERSION_MAJOR}::QXcbIntegrationPlugin IMPORTED_LOCATION_DEBUG)
    endif()
  endif()

  if(NOT EXISTS "${_qxcb_loc}")
    # Look beside QtCore, then in common system and multiarch plugin paths.
    get_target_property(_qtcore_loc Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_RELEASE)
    if(NOT _qtcore_loc)
      get_target_property(_qtcore_loc Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION)
    endif()
    if(NOT _qtcore_loc)
      get_target_property(_qtcore_loc Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_DEBUG)
    endif()
    get_filename_component(_qtcore_path "${_qtcore_loc}" DIRECTORY)
    find_file(_qxcb_loc "libqxcb.so"
              HINTS "${_qtcore_path}/../plugins/platforms"
              PATHS "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/qt/plugins/platforms"
                    "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/qt5/plugins/platforms"
                    "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/qt6/plugins/platforms"
                    "/usr/lib/qt/plugins/platforms"
                    "/usr/lib/qt5/plugins/platforms"
                    "/usr/lib/qt6/plugins/platforms")
    # Feel free to append paths to the above if more paths are needed.
  endif()

  if(EXISTS "${_qxcb_loc}")
    InstallPlatform("${_qxcb_loc}" "${_destination}" "${_dep_destination}")
  else()
    message(FATAL_ERROR "Unable to find QXcb library")
  endif()
endmacro()
