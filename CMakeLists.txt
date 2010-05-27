project(XtalOpt)

set(CMAKE_MODULE_PATH ${XtalOpt_SOURCE_DIR}/cmake/modules)

# Minimum version of CMake that can be used
cmake_minimum_required(VERSION 2.6)

# Find the Avogadro library if it is installed
find_package(Avogadro REQUIRED)

set(QT_MIN_VERSION "4.5.0")
find_package(Qt4 REQUIRED)
find_package(OpenBabel2 REQUIRED)

# Set up the build environment
include(${Avogadro_USE_FILE})
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${OPENBABEL2_INCLUDE_DIR}
	)

# This is a modified version of the avogadro_plugin macro
function(process_plugin plugin_name src_list)
  qt4_automoc(${src_list})
  if(NOT "${ARGV2}" STREQUAL "")
    # Process the UI file for this plugin
    qt4_wrap_ui(plugin_UIS_H ${ARGV2})
  endif(NOT "${ARGV2}" STREQUAL "")
  if(NOT "${ARGV3}" STREQUAL "")
    # Process the RC file and add it to the plugin
    qt4_add_resources(plugin_RC_SRCS ${ARGV3})
  endif(NOT "${ARGV3}" STREQUAL "")
  add_library(${plugin_name} SHARED ${src_list} ${plugin_UIS_H}
              ${plugin_RC_SRCS})
  target_link_libraries(${plugin_name} AvogadroCore AvogadroWidget)

  if(UNIX)
    add_custom_target("${CMAKE_PROJECT_NAME}.mf"
      COMMAND avopkg -wizard "${CMAKE_PROJECT_NAME}"
    )
    add_custom_target(manifest
      DEPENDS "${CMAKE_PROJECT_NAME}.mf"
      COMMAND avopkg -wizard "${CMAKE_PROJECT_NAME}"
    )
    add_custom_target("${CMAKE_PROJECT_NAME}.avo"
      COMMAND avopkg -pack "${CMAKE_PROJECT_NAME}.mf"
    )
    add_custom_target(package
      DEPENDS "${CMAKE_PROJECT_NAME}.avo"
    )
    add_custom_target(install_package
      COMMAND avopkg "${CMAKE_PROJECT_NAME}.avo"
      DEPENDS "${CMAKE_PROJECT_NAME}.avo"
    )
  endif(UNIX)

  install(TARGETS ${plugin_name} DESTINATION "${Avogadro_PLUGIN_DIR}/contrib")

  set_target_properties(${plugin_name} PROPERTIES
                        OUTPUT_NAME ${plugin_name}
                        PREFIX "")
endfunction(process_plugin)


link_directories(${OPENBABEL2_LIBRARIES_DIRS})

# Build your plugin using the default options
set( generic_SRCS 
     src/generic/optbase.cpp
     src/generic/queuemanager.cpp
     src/generic/structure.cpp
     src/generic/tracker.cpp
     src/generic/xtal.cpp
     src/generic/optimizer.cpp
     src/generic/bt.cpp)

qt4_automoc(${generic_SRCS})
add_library(globalsearch STATIC ${generic_SRCS})
target_link_libraries( globalsearch
		       ${LINK_LIBS} 
		       ${QT_QTCORE_LIBRARY} 
		       ${QT_QTGUI_LIBRARY} 
		       ${OPENBABEL2_LIBRARIES})

set( spglib_SRCS 
     src/spglib/bravais.c
     src/spglib/bravais_art.c
     src/spglib/cell.c
     src/spglib/debug.c
     src/spglib/mathfunc.c
     src/spglib/mathfunc.h
     src/spglib/pointgroup.c
     src/spglib/primitive.c
     src/spglib/spacegroup.c
     src/spglib/spacegroup.h
     src/spglib/spacegroup_data.c
     src/spglib/spacegroup_data.h
     src/spglib/spacegroup_database.c
     src/spglib/spacegroup_database.h
     src/spglib/spglib.c
     src/spglib/symmetry.c
     src/spglib/symmetry_kpoint.c)

add_library(spglib STATIC ${spglib_SRCS})
# Suppress spglib warnings
set_target_properties( spglib PROPERTIES
		       COMPILE_FLAGS "-w")

set( xtalopt_SRCS
     src/extension.cpp
     src/xtalopt/xtalopt.cpp
     src/xtalopt/genetic.cpp
     src/xtalopt/optimizers/xtaloptoptimizer.cpp
     src/xtalopt/optimizers/vasp.cpp
     src/xtalopt/optimizers/gulp.cpp
     src/xtalopt/optimizers/pwscf.cpp
     src/xtalopt/testing/xtalopttest.cpp
     src/xtalopt/ui/dialog.cpp
     src/xtalopt/ui/tab_init.cpp
     src/xtalopt/ui/tab_edit.cpp
     src/xtalopt/ui/tab_opt.cpp
     src/xtalopt/ui/tab_sys.cpp
     src/xtalopt/ui/tab_progress.cpp
     src/xtalopt/ui/tab_plot.cpp
     src/xtalopt/ui/tab_log.cpp)

set( xtalopt_UIS 
     src/xtalopt/ui/dialog.ui
     src/xtalopt/ui/tab_init.ui
     src/xtalopt/ui/tab_edit.ui
     src/xtalopt/ui/tab_opt.ui
     src/xtalopt/ui/tab_sys.ui
     src/xtalopt/ui/tab_progress.ui
     src/xtalopt/ui/tab_plot.ui
     src/xtalopt/ui/tab_log.ui)

process_plugin(xtalopt "${xtalopt_SRCS}" "${xtalopt_UIS}")

add_dependencies(xtalopt globalsearch spglib)

target_link_libraries( xtalopt
 		       QPlotWidget
 		       globalsearch
 		       spglib)



option( BUILD_TESTS
 	"Whether to compile the test suite as well as the main code."
 	OFF )

if(BUILD_TESTS)
   enable_testing()

add_executable(checkTemplates tests/checkTemplates.cpp)
target_link_libraries(checkTemplates xtalopt)
add_test(TemplateCheck checkTemplates)

endif(BUILD_TESTS)
