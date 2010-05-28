project(XtalOpt)

cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${XtalOpt_SOURCE_DIR}/cmake/modules)

set(QT_MIN_VERSION "4.5.0")
find_package(Qt4 REQUIRED)
find_package(OpenBabel2 REQUIRED)
find_package(Avogadro REQUIRED)

include(${Avogadro_USE_FILE})
include_directories( ${CMAKE_CURRENT_BINARY_DIR}
		     ${OPENBABEL2_INCLUDE_DIR})

link_directories(${OPENBABEL2_LIBRARIES_DIRS})

option( BUILD_TESTS
 	"Whether to compile the test suite as well as the main code."
 	OFF )

if(BUILD_TESTS)
   enable_testing()
endif(BUILD_TESTS)

# This is a modified version of the avogadro_plugin function that produces
# a SHARED library instead of a MODULE
function(process_plugin plugin_name src_list)
  string(REPLACE ".cpp" ".h" hdr_list "${src_list}")
  qt4_wrap_cpp(moc_files ${hdr_list})
  if(NOT "${ARGV2}" STREQUAL "")
    # Process the UI file for this plugin
    qt4_wrap_ui(plugin_UIS_H ${ARGV2})
  endif(NOT "${ARGV2}" STREQUAL "")
  if(NOT "${ARGV3}" STREQUAL "")
    # Process the RC file and add it to the plugin
    qt4_add_resources(plugin_RC_SRCS ${ARGV3})
  endif(NOT "${ARGV3}" STREQUAL "")
  add_library(${plugin_name} SHARED ${src_list} ${plugin_UIS_H}
              ${plugin_RC_SRCS} ${moc_files})
  target_link_libraries(${plugin_name} AvogadroCore AvogadroWidget)

  install(TARGETS ${plugin_name} DESTINATION "${Avogadro_PLUGIN_DIR}/contrib")

  set_target_properties(${plugin_name} PROPERTIES
                        OUTPUT_NAME ${plugin_name}
                        PREFIX "")
endfunction(process_plugin)

add_subdirectory(src)
add_subdirectory(tests)
