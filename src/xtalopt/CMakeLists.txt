INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

set( xtalopt_SRCS
     debug.cpp
     extension.cpp
     xtalopt.cpp
     genetic.cpp
     spgInit/spgInit.cpp
     structures/xtal.cpp
     optimizers/xtaloptoptimizer.cpp
     optimizers/vasp.cpp
     optimizers/gulp.cpp
     optimizers/pwscf.cpp
     optimizers/castep.cpp
     optimizers/siesta.cpp
     testing/xtalopttest.cpp
     ui/dialog.cpp
     ui/tab_init.cpp
     ui/tab_edit.cpp
     ui/tab_opt.cpp
     ui/tab_progress.cpp
     ui/tab_plot.cpp
     ui/tab_log.cpp)

set( xtalopt_UIS
     ui/dialog.ui
     ui/tab_init.ui
     ui/tab_opt.ui
     ui/tab_progress.ui
     ui/tab_plot.ui
     ui/tab_log.ui)

set( xtalopt_RCS xtalopt.qrc )

process_plugin(xtalopt "${xtalopt_SRCS}" "${xtalopt_UIS}" "${xtalopt_RCS}")

add_dependencies(xtalopt globalsearch XtalComp spglib)

# Use the C++11 standards
if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -Wno-sign-compare")
endif()

target_link_libraries( xtalopt
                       globalsearch
                       XtalComp
                       spglib)
