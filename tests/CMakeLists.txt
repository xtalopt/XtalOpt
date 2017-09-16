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

if(ENABLE_MOLECULAR)
  link_directories(${RDKit_LIBRARY_DIRS})
endif(ENABLE_MOLECULAR)

set(tests
  formats
  genetic
  optbase
  structure
  spglib
  randdouble
  randspg
  xtal
  xtaloptunit
)

if(ENABLE_MOLECULAR)
  set(tests
      ${tests}
      conformergenerator
      moltransformations
  )
endif(ENABLE_MOLECULAR)

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
    # We need to tell Windows exactly where to search for dynamically linked
    # libraries
    # ssh.dll should be in dir(${LIBSSH_LIBRARIES})\..\bin
    get_filename_component(LIBSSH_DLL_DIR "${LIBSSH_LIBRARIES}" DIRECTORY)
    set(DEP_SEARCH_DIRS "${DEP_SEARCH_DIRS}" "${LIBSSH_DLL_DIR}/../bin")

    # qwt.dll should be in the same dir as ${QWT_LIBRARIES}
    get_filename_component(QWT_DLL_DIR "${QWT_LIBRARIES}" DIRECTORY)
    set(DEP_SEARCH_DIRS "${DEP_SEARCH_DIRS}" "${QWT_DLL_DIR}")

    # All of the Qt dependencies will hopefully be together in the bin of the
    # root directory. We will need to change this part in the future if they
    # are not.
    get_target_property(QtCore_location Qt5::Core LOCATION)
    get_filename_component(QtCore_location "${QtCore_location}" DIRECTORY)
    set(DEP_SEARCH_DIRS "${DEP_SEARCH_DIRS}" "${QtCore_location}/../bin")

    include(MacroInstallDependencies)
    CopyDependencies("${TestsLocation}/${TestToCheck}.exe" "${TestsLocation}" "${DEP_SEARCH_DIRS}")
  endif()
endif(WIN32)
