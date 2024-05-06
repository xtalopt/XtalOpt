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
  formats
  genetic
  genxrd
  optbase
  structure
  spglib
  randdouble
  randspg
  xtal
  xtaloptunit
)

if (ENABLE_SSH)
set(tests
  ${tests}
  loadleveler
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

set(_link_libs
    xtalopt_static
    spglib
    XtalComp
    globalsearch
    Qt5::Test
)

if(ENABLE_SSH)
  set(_link_libs
      ${_link_libs}
      ${LIBSSH_LIBRARIES}
  )
endif(ENABLE_SSH)

foreach (test ${tests})
  message(STATUS "Test:  ${test}")
  set(test_SRCS ${test}test.cpp)
  add_executable(${test}test ${test_SRCS})
  target_link_libraries(${test}test ${_link_libs})

  add_test(${test}test ${CMAKE_BINARY_DIR}/tests/${test}test)
  set_property(TARGET ${test}test PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_property(SOURCE ${test_SRCS} PROPERTY LABELS globalsearch)
  set_property(TARGET ${test}test PROPERTY LABELS globalsearch)
  set_property(TEST ${test}test PROPERTY LABELS globalsearch)
endforeach ()

if(WIN32)
  set(TestsLocation "${CMAKE_BINARY_DIR}/tests")
  # We have to pick a test that uses all of the dependencies, or we will
  # have to use multiple tests to install all of the dependencies
  set(TestToCheck "spglibtest")

  if (NOT EXISTS "${TestsLocation}/${TestToCheck}.exe")
    message("-- warning: On Win32, BUILD_TESTS was set before tests were built")
    message("-- warning: CMake will be automatically called again after the tests are built in order to install the dependencies")
    add_custom_command(TARGET ${TestToCheck}
                       COMMAND "${CMAKE_COMMAND}" -E cmake_echo_color
                       "-- Re-running cmake to locate test dependencies"
                       POST_BUILD)
    add_custom_command(TARGET ${TestToCheck}
                       WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                       COMMAND "${CMAKE_COMMAND}" "${CMAKE_SOURCE_DIR}" POST_BUILD)
  else()
    include(MacroInstallDependencies)
    CopyDependencies("${TestsLocation}/${TestToCheck}.exe" "${TestsLocation}" "${DEP_SEARCH_DIRS}")
  endif()
endif(WIN32)
