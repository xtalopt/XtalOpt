INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

set( xtalopt_SRCS
     debug.cpp
     xtalopt.cpp
     genetic.cpp
     structures/xtal.cpp
     optimizers/xtaloptoptimizer.cpp
     optimizers/vasp.cpp
     optimizers/gulp.cpp
     optimizers/pwscf.cpp
     optimizers/castep.cpp
     optimizers/siesta.cpp
     testing/xtalopttest.cpp
     ui/dialog.cpp
     ui/randSpgDialog.cpp
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
     ui/tab_log.ui
     ui/randSpgDialog.ui)

set( xtalopt_RCS xtalopt.qrc )

# Use the C++11 standards
if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -Wno-sign-compare")
endif()

string(REPLACE ".cpp" ".h" hdr_list "${xtalopt_SRCS}")
# Process MOC files
qt4_wrap_cpp(moc_files ${hdr_list})
# Process the UI file for this plugin
qt4_wrap_ui(plugin_UIS_H ${xtalopt_UIS})
# Add resources
qt4_add_resources(plugin_RC_SRCS ${xtalopt_RCS})
add_executable(xtalopt main.cpp "${xtalopt_SRCS}" "${plugin_UIS_H}"
                       "${plugin_RC_SRCS}" "${moc_files}")

target_link_libraries(xtalopt globalsearch XtalComp spglib RandSpg avogadro)
