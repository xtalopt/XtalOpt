INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

set( xtalopt_SRCS
     debug.cpp
     extension.cpp
     genetic.cpp
     holefinder.cpp
     #mxtaloptgenetic.cpp
     molecularxtalmutator.cpp
     molecularxtaloptimizer.cpp
     molecularxtalsupercellgenerator.cpp
     optimizers/castep.cpp
     optimizers/gulp.cpp
     optimizers/mopac.cpp
     optimizers/openbabeloptimizer.cpp
     optimizers/pwscf.cpp
     optimizers/vasp.cpp
     optimizers/xtaloptoptimizer.cpp
     queueinterfaces/openbabel.cpp
     queueinterfaces/openbabeldialog.cpp
     structures/molecularxtal.cpp
     structures/submolecule.cpp
     structures/submoleculesource.cpp
     structures/xtal.cpp
     submoleculeranker.cpp
     testing/xtalopttest.cpp
     ui/dialog.cpp
     ui/mxtalpreoptconfigdialog.cpp
     ui/tab_edit.cpp
     ui/tab_init.cpp
     ui/tab_log.cpp
     ui/tab_opt.cpp
     ui/tab_plot.cpp
     ui/tab_progress.cpp
     xtalopt.cpp
)

set( xtalopt_UIS
     ui/dialog.ui
     ui/mxtalpreoptconfigdialog.ui
     ui/tab_init.ui
     ui/tab_log.ui
     ui/tab_opt.ui
     ui/tab_plot.ui
     ui/tab_progress.ui
)

set( xtalopt_RCS xtalopt.qrc )

process_plugin(xtalopt "${xtalopt_SRCS}" "${xtalopt_UIS}" "${xtalopt_RCS}")

add_dependencies(xtalopt globalsearch XtalComp spglib)

target_link_libraries( xtalopt
                       globalsearch
                       XtalComp
                       spglib 
                       ${LIBSSH_LIBRARIES}
                       )
