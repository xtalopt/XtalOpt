# Most of this is borrowed from Avogadro's tests CMakeLists.txt

include(${QT_USE_FILE})

# define TESTDATADIR for tests that need input files
cmake_policy(SET CMP0005 NEW) # needed for line below (escapes value)
add_definitions( -DTESTDATADIR="${CMAKE_SOURCE_DIR}/tests/sampledata/")

# Ensure the XtalOpt include directory is always first
get_directory_property(tmp_include_dirs INCLUDE_DIRECTORIES)
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES
  "${XtalOpt_SOURCE_DIR}/src/;${tmp_include_dirs}")
include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${EIGEN2_INCLUDE_DIR}
  ${OPENBABEL2_INCLUDE_DIR}
)

link_directories(
  ${XtalOpt_BINARY_DIR}/src
  ${OPENBABEL2_LIBRARY_DIRS}
)

set(tests
  holefinder
  molecularxtal
  molecularxtalmutator
  molecularxtaloptimizer
  molecularxtalsupercellgenerator
  #mxtaloptgenetic
  optbase
  submoleculeranker
  structure
  spglib
  submolecule
  submoleculesource
  randdouble
  xtal
  xtaloptunit
)

if (ENABLE_SSH)
set( tests
  ${tests}
  loadleveler
)
if (USE_CLI_SSH)
set( tests
  ${tests}
  sshconnection_cli
  sshmanager_cli
)
else (USE_CLI_SSH)
set( tests
  ${tests}
  sshconnection_libssh
  sshmanager_libssh
)
endif (USE_CLI_SSH)

endif(ENABLE_SSH)

foreach (test ${tests})
  message(STATUS "Test:  ${test}")
  set(test_SRCS ${test}test.cpp)
  set(test_MOC_CPPS ${test}test.cpp)
  QT4_WRAP_CPP(test_MOC_SRCS ${test_MOC_CPPS})
  ADD_CUSTOM_TARGET(${test}testmoc ALL DEPENDS ${test_MOC_SRCS})
  add_executable(${test}test ${test_SRCS})
  add_dependencies(${test}test ${test}testmoc)
  target_link_libraries(${test}test
    ${OPENBABEL2_LIBRARIES}
    ${QT_LIBRARIES}
    ${QT_QTTEST_LIBRARY}
    xtalopt_static
    spglib
    XtalComp
    avogadro
    globalsearch)
  if (ENABLE_SSH)
    target_link_libraries(${test}test ${LIBSSH_LIBRARIES})
  endif()
  add_test(${test}Test ${CMAKE_BINARY_DIR}/tests/${test}test)
  set_property(SOURCE ${test_SRCS} PROPERTY LABELS globalsearch)
  set_property(TARGET ${test}test PROPERTY LABELS globalsearch)
  set_property(TEST ${test}Test PROPERTY LABELS globalsearch)
endforeach ()
