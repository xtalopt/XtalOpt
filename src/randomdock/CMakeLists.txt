
# Example project building an external plugin against installed Avogadro
# library. See http://avogadro.openmolecules.net/ for more information.

# Project name - plugin name or something meaningful
project(RandomDock)

set(CMAKE_MODULE_PATH ${RandomDock_SOURCE_DIR}/cmake/modules)

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
link_directories(${OPENBABEL2_LIBRARY_DIRS})

# Build your plugin using the default options
set (randomdock_SRCS randomdockdialog.cpp
    		     scenemol.cpp
    		     matrixmol.cpp
		     substratemol.cpp
		     randomdockextension.cpp
		     randomdockGAMESS.cpp
		     templates.cpp
		     randomdock.cpp
		     tab_init.cpp
		     tab_edit.cpp
		     tab_params.cpp
		     tab_conformers.cpp
		     tab_sys.cpp
		     tab_results.cpp
		     tab_plot.cpp
		     tab_log.cpp)
set (randomdock_UIS randomdockdialog.ui
    		    tab_init.ui
		    tab_conformers.ui
		    tab_edit.ui
		    tab_params.ui
		    tab_sys.ui
		    tab_results.ui
		    tab_plot.ui
		    tab_log.ui)

avogadro_plugin(randomdock 
	"${randomdock_SRCS}"
	"${randomdock_UIS}"
			
)
