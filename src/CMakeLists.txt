include(${CMAKE_SOURCE_DIR}/cmake/XtalOptWarnings.cmake)
xtalopt_enable_warnings()

# Use project definitions only in src/ and tests/ (not in external/).
add_compile_definitions(${XTALOPT_COMPILE_DEFINITIONS})

add_subdirectory(common)
add_subdirectory(atoms)
add_subdirectory(generatexrd)
add_subdirectory(search)
add_subdirectory(xtalopt)
