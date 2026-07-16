# Written by Patrick Avery - 2017
# This is a macro for installing Cocoa during a Mac build

macro(InstallCocoa _destination _qt_plugins_var)
  include(MacroInstallQtPlugin)
  install_qt_plugin("Qt${QT_VERSION_MAJOR}::QCocoaIntegrationPlugin"
                     "${_destination}" "${_qt_plugins_var}")
endmacro()
