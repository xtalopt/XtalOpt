INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/gapc)

set( gapc_SRCS
     extension.cpp
     gapc.cpp
     genetic.cpp
     structures/cluster.cpp
     structures/protectedcluster.cpp
     optimizers/adf.cpp
     optimizers/gulp.cpp
     ui/dialog.cpp
     ui/tab_init.cpp
     ui/tab_edit.cpp
     ui/tab_opt.cpp
     ui/tab_progress.cpp
     ui/tab_plot.cpp
     ui/tab_log.cpp
)

set( gapc_UIS
     ui/dialog.ui
     ui/tab_init.ui
     ui/tab_opt.ui
     ui/tab_progress.ui
     ui/tab_plot.ui
     ui/tab_log.ui
)

process_plugin(gapc "${gapc_SRCS}" "${gapc_UIS}")

add_dependencies(gapc globalsearch)

target_link_libraries( gapc
 		       globalsearch)
