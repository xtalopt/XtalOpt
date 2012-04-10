INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/examplesearch)

set( examplesearch_SRCS
     examplesearch.cpp
     extension.cpp
     optimizers/gamess.cpp
     optimizers/mopac.cpp
     ui/dialog.cpp
     ui/tab_init.cpp
     ui/tab_edit.cpp
     ui/tab_progress.cpp
     ui/tab_plot.cpp
     ui/tab_log.cpp
)

set( examplesearch_UIS
     ui/dialog.ui
     ui/tab_init.ui
     ui/tab_progress.ui
     ui/tab_plot.ui
     ui/tab_log.ui
)

process_plugin(examplesearch "${examplesearch_SRCS}" "${examplesearch_UIS}")

add_dependencies(examplesearch globalsearch)

target_link_libraries(examplesearch globalsearch)
