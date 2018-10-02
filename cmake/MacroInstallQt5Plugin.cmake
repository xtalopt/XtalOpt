# Written by Patrick Avery - 2017
# This is a macro for installing Qt Plugins

macro(install_qt5_plugin _qt_plugin_name _destination _qt_plugins_var)
  get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
  if(EXISTS "${_qt_plugin_path}")
    get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
    get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
    get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
    set(_qt_plugin_dest "${_destination}/${_qt_plugin_type}")
    install(FILES "${_qt_plugin_path}"
      DESTINATION "${_qt_plugin_dest}")
    set(${_qt_plugins_var}
      "${${_qt_plugins_var}};${_qt_plugin_dest}/${_qt_plugin_file}")
  else()
    message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
  endif()
endmacro()
