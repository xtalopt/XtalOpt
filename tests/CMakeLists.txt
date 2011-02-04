# Most of this is borrowed from Avogadro's tests CMakeLists.txt

include(${QT_USE_FILE})

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

set( tests
  optbase
  structure
  sshconnection
  sshmanager
  spglib
  randdouble
)

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
    ssh_static
    avogadro
    globalsearch)
  add_test(${test}Test ${CMAKE_BINARY_DIR}/tests/${test}test)
  set_property(SOURCE ${test_SRCS} PROPERTY LABELS globalsearch)
  set_property(TARGET ${test}test PROPERTY LABELS globalsearch)
  set_property(TEST ${test}Test PROPERTY LABELS globalsearch)
endforeach ()
