# Example project building an external plugin against installed Avogadro
# library. See http://avogadro.openmolecules.net/ for more information.

# Project name - plugin name or something meaningful
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
    	src/xtal.cpp
	src/extension.cpp
	src/templates.cpp
	src/xtalopt.cpp
	src/genetic.cpp
	src/optimizers.cpp
	src/optimizers/vasp.cpp
	src/optimizers/gulp.cpp
	src/optimizers/pwscf.cpp
	src/testing/xtalopttest.cpp
	src/ui/dialog.cpp
	src/ui/tab_init.cpp
	src/ui/tab_edit.cpp
	src/ui/tab_opt.cpp
	src/ui/tab_sys.cpp
	src/ui/tab_progress.cpp
	src/ui/tab_plot.cpp
	src/ui/tab_log.cpp
	src/bt.cpp)

set (xtalopt_UIS 
        src/ui/dialog.ui
	src/ui/tab_init.ui
	src/ui/tab_edit.ui
	src/ui/tab_opt.ui
	src/ui/tab_sys.ui
	src/ui/tab_progress.ui
	src/ui/tab_plot.ui
	src/ui/tab_log.ui)

avogadro_plugin(xtalopt 
	"${xtalopt_SRCS}"
	"${xtalopt_UIS}"
)
