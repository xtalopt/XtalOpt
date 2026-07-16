include(${CMAKE_SOURCE_DIR}/cmake/XtalOptWarnings.cmake)
xtalopt_enable_warnings()

# Use the project definitions from the root CMake file.
add_compile_definitions(${XTALOPT_COMPILE_DEFINITIONS})

# Some of this is borrowed from Avogadro's tests CMakeLists.txt

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

# One entry per test: its libraries and extra CTest labels. Every test gets
# search and unit labels.
set(fileutils_test_libs           common_core)
set(formats_test_libs             search_core)
set(genetic_test_libs             xtalopt_core)
set(genxrd_test_libs              generatexrd_core search_core)
set(hull_test_libs                common_math)
set(atoms_test_libs               atoms_core)
set(searchbase_test_libs          search_core)
set(searchbase_test_labels        queue)
set(structure_test_libs           xtalopt_core)
set(spglib_test_libs              xtalopt_core)
set(xtal_test_libs                xtalopt_core)
set(xtaloptunit_test_libs         xtalopt_core)
set(xtaloptunit_test_labels       xtalopt options state)
set(legacycompat_test_libs        xtalopt_core)
set(legacycompat_test_labels      xtalopt legacy)
set(passwordprompt_test_libs      xtalopt_core search_cli)
set(sshconnection_cli_test_libs   search_core)
set(sshmanager_cli_test_libs      search_core)
set(sshconnection_libssh_test_libs search_core)
set(sshmanager_libssh_test_libs   search_core)

set(tests
  fileutils
  formats
  genetic
  genxrd
  hull
  atoms
  searchbase
  structure
  spglib
  xtal
  xtaloptunit
  legacycompat
  passwordprompt
  sshconnection_cli
  sshmanager_cli
)

if(ENABLE_LIBSSH)
  list(APPEND tests
    sshconnection_libssh
    sshmanager_libssh
  )
endif()

foreach (test ${tests})
  message(STATUS "Test:  ${test}")
  set(test_SRCS ${test}test.cpp)
  add_executable(${test}test ${test_SRCS})
  set(test_link_libs Qt${QT_VERSION_MAJOR}::Test ${${test}_test_libs})
  target_include_directories(${test}test PRIVATE
    ${XtalOpt_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_CURRENT_BINARY_DIR}
  )
  if(ENABLE_LIBSSH)
    target_include_directories(${test}test PRIVATE
      ${LIBSSH_INCLUDE_DIRS}
    )
  endif()
  target_compile_definitions(${test}test PRIVATE
    TESTDATADIR="${CMAKE_SOURCE_DIR}/tests/sampledata/")
  target_link_libraries(${test}test ${test_link_libs})
  set_target_properties(${test}test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin"
    AUTOGEN_BUILD_DIR "${CMAKE_BINARY_DIR}/tests/autogen/${test}test"
  )

  set(test_labels search unit ${${test}_test_labels})
  if(test MATCHES "ssh" OR test STREQUAL "passwordprompt")
    list(APPEND test_labels ssh)
  endif()
  if(test MATCHES "_libssh$")
    list(APPEND test_labels libssh)
  endif()

  add_test(NAME ${test}test COMMAND $<TARGET_FILE:${test}test>)
  set_property(TARGET ${test}test PROPERTY LABELS ${test_labels})
  set_property(TEST ${test}test PROPERTY LABELS ${test_labels})

  if(WIN32)
    # Use one dependency-copy script per build configuration for Visual Studio.
    set(_copy_test_deps_script
        "${CMAKE_CURRENT_BINARY_DIR}/copy_${test}test_runtime_dependencies_$<CONFIG>.cmake")
    file(GENERATE OUTPUT "${_copy_test_deps_script}" CONTENT
"set(XTALOPT_RUNTIME_DEPS_MODULE_PATH [==[${CMAKE_SOURCE_DIR}/cmake]==])
set(XTALOPT_RUNTIME_DEPS_EXE [==[$<TARGET_FILE:${test}test>]==])
set(XTALOPT_RUNTIME_DEPS_DESTINATION [==[$<TARGET_FILE_DIR:${test}test>]==])
set(XTALOPT_RUNTIME_DEPS_SEARCH_DIRS [==[${DEP_SEARCH_DIRS}]==])
include([==[${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake]==])
")
    add_custom_command(TARGET ${test}test POST_BUILD
                       COMMAND "${CMAKE_COMMAND}" -P "${_copy_test_deps_script}"
                       VERBATIM)
  endif()
endforeach ()
