set(search_core_SRCS
    search.cpp
    selection.cpp
    optsteps.cpp
    search_objectives.cpp
    search_templates.cpp
    queuemanager.cpp
    structure.cpp
    tracker.cpp
    optimizer.cpp
    optimizers/vaspoptimizer.cpp
    optimizers/pwscfoptimizer.cpp
    optimizers/castepoptimizer.cpp
    optimizers/siestaoptimizer.cpp
    optimizers/gulpoptimizer.cpp
    optimizers/mtpoptimizer.cpp
    slottedwaitcondition.cpp
    queueinterface.cpp
    queueinterfaces/directrun.cpp
    queueinterfaces/batch.cpp
    queueinterfaces/loadleveler.cpp
    queueinterfaces/lsf.cpp
    queueinterfaces/pbs.cpp
    queueinterfaces/sge.cpp
    queueinterfaces/slurm.cpp
    ssh/sshconnection_cli.cpp
    ssh/sshmanager_cli.cpp
    ssh/sshconnection.cpp
    ssh/sshmanager.cpp)

if(ENABLE_LIBSSH)
  list(APPEND search_core_SRCS
       ssh/sshconnection_libssh.cpp
       ssh/sshmanager_libssh.cpp)
endif()

set(search_gui_SRCS
    gui/abstractdialog.cpp
    gui/abstracttab.cpp
    gui/abstractopttab.cpp
    gui/defaultopttab.cpp
    gui/config_dialogs.cpp
    gui/guiinterface.cpp
    gui/optimizerdialog.cpp
    gui/queueinterfaces/globalqueueinterfacesettingswidget.cpp
    gui/queueinterfaces/loadlevelerdialog.cpp
    gui/queueinterfaces/lsfdialog.cpp
    gui/queueinterfaces/pbsdialog.cpp
    gui/queueinterfaces/sgedialog.cpp
    gui/queueinterfaces/slurmdialog.cpp)

# Use system includes for third-party headers (excluded from our warning handling).
set(_search_external_include_dirs ${CMAKE_SOURCE_DIR}/external)

if(ENABLE_LIBSSH)
  list(APPEND _search_external_include_dirs ${LIBSSH_INCLUDE_DIRS})
endif()

add_library(search_core STATIC ${search_core_SRCS})
add_library(search_cli STATIC
    cliinterface.cpp
    passwordprompt.cpp)

target_include_directories(search_core SYSTEM PRIVATE ${_search_external_include_dirs})
target_include_directories(search_cli SYSTEM PRIVATE ${_search_external_include_dirs})
target_link_libraries(search_core
  PUBLIC
    atoms_core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Threads::Threads)

if(ENABLE_LIBSSH)
  target_link_libraries(search_core PUBLIC ${LIBSSH_LIBRARIES})
endif()

target_link_libraries(search_cli
  PUBLIC
    search_core)

if(BUILD_XTALOPT_GUI)
  add_library(search_gui STATIC ${search_gui_SRCS})
  target_include_directories(search_gui SYSTEM PRIVATE
    ${_search_external_include_dirs})
  target_link_libraries(search_gui
    PUBLIC
      search_core
      common_gui
      Qwt::Qwt)
endif()
