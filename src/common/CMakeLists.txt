add_library(common_core STATIC
  fileutils.cpp)

# Set the project include root (other targets inherit this by linking common_core).
target_include_directories(common_core PUBLIC
  ${XtalOpt_SOURCE_DIR}/src)

target_link_libraries(common_core
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core)

add_library(common_math STATIC
  chull.cpp)

target_include_directories(common_math SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/external
  ${CMAKE_SOURCE_DIR}/external/qhull/src)

target_link_libraries(common_math
  PUBLIC
    common_core
  PRIVATE
    qhullcpp
    qhullstatic_r)

if(BUILD_XTALOPT_GUI)
  add_library(common_gui INTERFACE)
  target_link_libraries(common_gui
    INTERFACE
      common_core
      Qt${QT_VERSION_MAJOR}::Widgets)
endif()
