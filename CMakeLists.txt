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

link_directories(${OPENBABEL2_LIBRARIES_DIRS})

# Build your plugin using the default options
set (xtalopt_SRCS 
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
	src/spglib/symmetry_kpoint.c
    	src/generic/optbase.cpp
    	src/generic/structure.cpp
    	src/generic/xtal.cpp
	src/extension.cpp
	src/generic/tracker.cpp
	src/xtalopt/xtalopt.cpp
	src/generic/queuemanager.cpp
	src/xtalopt/genetic.cpp
	src/generic/optimizer.cpp
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
	src/xtalopt/ui/tab_log.cpp
	src/generic/bt.cpp)

set (xtalopt_UIS 
        src/xtalopt/ui/dialog.ui
	src/xtalopt/ui/tab_init.ui
	src/xtalopt/ui/tab_edit.ui
	src/xtalopt/ui/tab_opt.ui
	src/xtalopt/ui/tab_sys.ui
	src/xtalopt/ui/tab_progress.ui
	src/xtalopt/ui/tab_plot.ui
	src/xtalopt/ui/tab_log.ui)

avogadro_plugin(xtalopt 
	"${xtalopt_SRCS}"
	"${xtalopt_UIS}"
)

target_link_libraries(xtalopt QPlotWidget)

option( BUILD_TESTS
	"Whether to compile the test suite as well as the main code."
	OFF )

if(BUILD_TESTS)
  enable_testing()

  add_executable(checkTemplates ${xtalopt_SRCS} tests/checkTemplates.cpp)
  target_link_libraries(checkTemplates ${LINK_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${OPENBABEL2_LIBRARIES} avogadro QPlotWidget)
  add_test(TemplateCheck checkTemplates)

endif(BUILD_TESTS)
