include(CheckIncludeFiles)
INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/globalsearch/)
INCLUDE_DIRECTORIES(${LIBSSH_INCLUDE_DIRS})

set( globalsearch_SRCS
     optbase.cpp
     queuemanager.cpp
     eleminfo.cpp
     structure.cpp
     tracker.cpp
     optimizer.cpp
     optimizerdialog.cpp
     bt.cpp
     slottedwaitcondition.cpp
     ui/abstractdialog.cpp
     ui/abstracttab.cpp
     ui/abstractedittab.cpp
     ui/defaultedittab.cpp
     queueinterface.cpp
     queueinterfaces/local.cpp
     queueinterfaces/localdialog.cpp
     utilities/fileutils.cpp
     structures/unitcell.cpp
     formats/formats.cpp
     formats/gulpformat.cpp
     formats/vaspformat.cpp
)

if (ENABLE_SSH)
if (NOT USE_CLI_SSH)
set( globalsearch_SRCS
     ${globalsearch_SRCS}
     sshconnection_libssh.cpp
     sshmanager_libssh.cpp
)
else (NOT USE_CLI_SSH)
set( globalsearch_SRCS
     ${globalsearch_SRCS}
     sshconnection_cli.cpp
     sshmanager_cli.cpp
)
endif (NOT USE_CLI_SSH)
set( globalsearch_SRCS
     ${globalsearch_SRCS}
     sshconnection.cpp
     sshmanager.cpp
     queueinterfaces/remote.cpp
     queueinterfaces/loadleveler.cpp
     queueinterfaces/loadlevelerdialog.cpp
     queueinterfaces/lsf.cpp
     queueinterfaces/lsfdialog.cpp
     queueinterfaces/pbs.cpp
     queueinterfaces/pbsdialog.cpp
     queueinterfaces/sge.cpp
     queueinterfaces/sgedialog.cpp
     queueinterfaces/slurm.cpp
     queueinterfaces/slurmdialog.cpp
)
endif()

set( globalsearch_UIS
     ui/defaultedittab.ui
)

if (ENABLE_SSH)
set( globalsearch_UIS
     ${globalsearch_UIS}
     queueinterfaces/loadlevelerdialog.ui
     queueinterfaces/lsfdialog.ui
     queueinterfaces/pbsdialog.ui
     queueinterfaces/sgedialog.ui
     queueinterfaces/slurmdialog.ui
)
endif()


string(REPLACE ".cpp" ".h" hdr_list "${globalsearch_SRCS}")
qt5_wrap_cpp(moc_files ${hdr_list})
qt5_wrap_ui(globalsearch_UIS_H ${globalsearch_UIS})
add_library(globalsearch STATIC ${globalsearch_SRCS} ${globalsearch_UIS} ${moc_files})
target_link_libraries( globalsearch
		       ${LINK_LIBS}
                       Qt5::Concurrent
                       Qt5::Widgets)

if (ENABLE_SSH AND NOT USE_CLI_SSH)
  target_link_libraries(globalsearch ${LIBSSH_LIBRARIES})
endif()

# This is needed for backtraces
check_include_files(execinfo.h HAVE_EXECINFO_H)
if(HAVE_EXECINFO_H)
  add_definitions(-D_HAVE_EXECINFO_H_)
endif()

# Get rid of a few annoying warnings (the second one shows
# up on some clang compilers with Eigen3)
if(UNIX)
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-deprecated-register")
endif(UNIX)
