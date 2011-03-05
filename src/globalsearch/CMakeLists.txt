include(CheckIncludeFiles)
INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/globalsearch/)

set( globalsearch_SRCS
     optbase.cpp
     queuemanager.cpp
     structure.cpp
     tracker.cpp
     optimizer.cpp
     optimizerdialog.cpp
     bt.cpp
     sshconnection.cpp
     sshmanager.cpp
     random.cpp
     ui/abstractdialog.cpp
     ui/abstracttab.cpp
     ui/abstractedittab.cpp
     ui/defaultedittab.cpp
     queueinterface.cpp
     queueinterfaces/local.cpp
     queueinterfaces/localdialog.cpp
     queueinterfaces/remote.cpp
     queueinterfaces/pbs.cpp
     queueinterfaces/pbsdialog.cpp
     queueinterfaces/sge.cpp
     queueinterfaces/sgedialog.cpp
)

set( globalsearch_UIS
     ui/defaultedittab.ui
     queueinterfaces/pbsdialog.ui
     queueinterfaces/sgedialog.ui
)

string(REPLACE ".cpp" ".h" hdr_list "${globalsearch_SRCS}")
qt4_wrap_cpp(moc_files ${hdr_list})
qt4_wrap_ui(globalsearch_UIS_H ${globalsearch_UIS})
add_library(globalsearch STATIC ${globalsearch_SRCS} ${globalsearch_UIS} ${moc_files})
target_link_libraries( globalsearch
		       ssh_static
		       ${LIBSSH_LINK_LIBRARIES}
		       ${LINK_LIBS}
		       ${QT_QTCORE_LIBRARY}
		       ${QT_QTGUI_LIBRARY}
		       ${OPENBABEL2_LIBRARIES})

add_dependencies(globalsearch ssh_static)

# This is needed for backtraces
check_include_files(execinfo.h HAVE_EXECINFO_H)
if(HAVE_EXECINFO_H)
  add_definitions(-D_HAVE_EXECINFO_H_)
endif()

