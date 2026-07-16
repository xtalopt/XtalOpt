add_library(atoms_core STATIC
  eleminfo.cpp
  generators.cpp
  molecule.cpp
  geometry.cpp
  basis/unitcell.cpp
  formats/castepformat.cpp
  formats/cifformat.cpp
  formats/cmlformat.cpp
  formats/formats.cpp
  formats/gulpformat.cpp
  formats/mtpformat.cpp
  formats/poscarformat.cpp
  formats/pwscfformat.cpp
  formats/siestaformat.cpp
  formats/xyzformat.cpp)

target_include_directories(atoms_core SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/external)

target_link_libraries(atoms_core
  PUBLIC
    common_core
    Qt${QT_VERSION_MAJOR}::Core
  PRIVATE
    common_math
    RandSpg
    msym
    spglib
    XtalComp)

if(BUILD_XTALOPT_GUI)
  add_library(atoms_gui STATIC
    gui/structureviewdialog.cpp)

  target_link_libraries(atoms_gui
    PUBLIC
      atoms_core
      common_gui)
endif()
