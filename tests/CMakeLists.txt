# Most of this is borrowed from Avogadro's tests CMakeLists.txt

find_package(Qt5 COMPONENTS Test REQUIRED)

# define TESTDATADIR for tests that need input files
add_definitions(-DTESTDATADIR="${CMAKE_SOURCE_DIR}/tests/sampledata/")

include_directories(
  ${XtalOpt_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${EIGEN3_INCLUDE_DIR}
)

link_directories(
  ${XtalOpt_BINARY_DIR}/src
)

set(tests
#  optbase
#  structure
  spglib
  randdouble
#  xtal
#  xtaloptunit
)

if (ENABLE_SSH)
set( tests
  ${tests}
#  loadleveler
)
if (USE_CLI_SSH)
set( tests
  ${tests}
#  sshconnection_cli
#  sshmanager_cli
)
else (USE_CLI_SSH)
set( tests
  ${tests}
#  sshconnection_libssh
#  sshmanager_libssh
)
endif (USE_CLI_SSH)

endif(ENABLE_SSH)

foreach (test ${tests})
  message(STATUS "Test:  ${test}")
  set(test_SRCS ${test}test.cpp)
  add_executable(${test}test ${test_SRCS})
  target_link_libraries(${test}test
    xtalopt_static
    spglib
    XtalComp
    globalsearch
    Qt5::Test)
  if (ENABLE_SSH)
    target_link_libraries(${test}test ${LIBSSH_LIBRARIES})
  endif()
  add_test(${test}test ${CMAKE_BINARY_DIR}/tests/${test}test)
  set_property(TARGET ${test}test PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_property(SOURCE ${test_SRCS} PROPERTY LABELS globalsearch)
  set_property(TARGET ${test}test PROPERTY LABELS globalsearch)
  set_property(TEST ${test}test PROPERTY LABELS globalsearch)
endforeach ()
