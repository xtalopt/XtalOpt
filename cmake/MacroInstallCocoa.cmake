# Written by Patrick Avery - 2017
# This is a macro for installing Cocoa during a Mac build

macro(InstallCocoa _destination _qt_plugins_var)
  include(MacroInstallQt5Plugin)
  install_qt5_plugin("Qt5::QCocoaIntegrationPlugin"
                     "${_destination}" "${_qt_plugins_var}")
endmacro()
