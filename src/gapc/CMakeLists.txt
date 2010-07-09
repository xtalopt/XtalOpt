INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/gapc)

set( gapc_SRCS
     extension.cpp
     gapc.cpp
     structures/cluster.cpp
     structures/protectedcluster.cpp
     ui/dialog.cpp
)

set( gapc_UIS
     ui/dialog.ui
)

process_plugin(gapc "${gapc_SRCS}" "${gapc_UIS}")

add_dependencies(gapc globalsearch)

target_link_libraries( gapc
 		       globalsearch)
