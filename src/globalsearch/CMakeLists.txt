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
     utilities/passwordprompt.cpp
     structures/molecule.cpp
     structures/unitcell.cpp
     formats/formats.cpp
     formats/obconvert.cpp
     formats/castepformat.cpp
     formats/cmlformat.cpp
     formats/genericformat.cpp
     formats/gulpformat.cpp
     formats/poscarformat.cpp
     formats/pwscfformat.cpp
     formats/siestaformat.cpp
     formats/vaspformat.cpp
     formats/xyzformat.cpp
     formats/zmatrixformat.cpp
     http/aflowml.cpp
     http/httprequestmanager.cpp
     xrd/generatexrd.cpp
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
     queueinterfaces/globalqueueinterfacesettingswidget.cpp
)
endif()

add_library(globalsearch STATIC ${globalsearch_SRCS} ${moc_files}
  constants.h)

set(GLOBALSEARCH_LINK_LIBS
    ${GLOBALSEARCH_LINK_LIBS}
    Qt5::Concurrent
    Qt5::Network # Needed for XtalOpt RPC
    Qt5::Widgets
    pugixml
    ${CMAKE_THREAD_LIBS_INIT} # Standard threads library
)

target_link_libraries(globalsearch ${GLOBALSEARCH_LINK_LIBS})

if (ENABLE_SSH AND NOT USE_CLI_SSH)
  target_link_libraries(globalsearch ${LIBSSH_LIBRARIES})
endif()

# This is needed for backtraces
check_include_files(execinfo.h HAVE_EXECINFO_H)
if(HAVE_EXECINFO_H)
  add_definitions(-D_HAVE_EXECINFO_H_)
endif()

# Get rid of a few annoying warnings. In the future, we can probably
# just try to fix the warnings rather than suppress them... except
# for the clang one. That is caused by Eigen.
if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif(UNIX)
# This one shows up on some clang compilers with Eigen3
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
endif()
if(MSVC)
  # Ignore sign comparison warning
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018")
endif(MSVC)
