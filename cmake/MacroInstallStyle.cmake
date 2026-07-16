# Written by Patrick Avery - 2018
# This is a macro for installing a Qt style during a build
# Currently, it is used for qmacstyle and qwindowsvistastyle

macro(InstallQMacStyle _destination _qt_plugins_var)
  include(MacroInstallQtPlugin)
  install_qt_plugin("Qt${QT_VERSION_MAJOR}::QMacStylePlugin"
                     "${_destination}" "${_qt_plugins_var}")
endmacro()

macro(InstallQWindowsVistaStyle _destination _qt_plugins_var)
  include(MacroInstallQtPlugin)
  # Qt6.7+ renamed the windowsvista style plugin target.
  if(TARGET "Qt${QT_VERSION_MAJOR}::QModernWindowsStylePlugin")
    install_qt_plugin("Qt${QT_VERSION_MAJOR}::QModernWindowsStylePlugin"
                       "${_destination}" "${_qt_plugins_var}")
  else()
    install_qt_plugin("Qt${QT_VERSION_MAJOR}::QWindowsVistaStylePlugin"
                       "${_destination}" "${_qt_plugins_var}")
  endif()
endmacro()
