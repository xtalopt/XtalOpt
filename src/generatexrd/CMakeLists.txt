add_library(generatexrd_core STATIC
  generatexrd.cpp)

target_link_libraries(generatexrd_core
  PUBLIC
    atoms_core
  PRIVATE
    xraylib)

if(BUILD_XTALOPT_GUI)
  add_library(generatexrd_gui STATIC
    gui/xrd_plot.cpp
    gui/xrdviewdialog.cpp)

  target_link_libraries(generatexrd_gui
    PUBLIC
      generatexrd_core
      common_gui
      Qwt::Qwt)
endif()
