INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/randomdock)

set( randomdock_SRCS
     extension.cpp
     randomdock.cpp
     optimizers/adf.cpp
     optimizers/gamess.cpp
     optimizers/mopac.cpp
     optimizers/gaussian.cpp
     structures/scene.cpp
     structures/matrix.cpp
     structures/substrate.cpp
     ui/dialog.cpp
     ui/tab_init.cpp
     ui/tab_edit.cpp
     ui/tab_params.cpp
     ui/tab_conformers.cpp
     ui/tab_progress.cpp
     ui/tab_plot.cpp
     ui/tab_log.cpp
)

set( randomdock_UIS
     ui/dialog.ui
     ui/tab_init.ui
     ui/tab_conformers.ui
     ui/tab_params.ui
     ui/tab_progress.ui
     ui/tab_plot.ui
     ui/tab_log.ui
)

process_plugin(randomdock "${randomdock_SRCS}" "${randomdock_UIS}")

add_dependencies(randomdock globalsearch)

target_link_libraries( randomdock
 		       globalsearch)
