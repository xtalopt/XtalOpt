cmake_minimum_required(VERSION 3.12)

project(libmsym C)

set(LIBMSYM_SOURCES
  src/context.c
  src/debug.c
  src/elements.c
  src/equivalence_set.c
  src/geometry.c
  src/linalg.c
  src/msym.c
  src/msym_error.c
  src/permutation.c
  src/point_group.c
  src/symmetrize.c
  src/symmetry.c
  src/symop.c
)

add_library(msym STATIC ${LIBMSYM_SOURCES})

set_target_properties(msym PROPERTIES
  LINKER_LANGUAGE C
)

target_include_directories(msym PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(msym PUBLIC
  MSYM_EXPORTS_BUILT_AS_STATIC
)

if(NOT MSVC)
  target_compile_options(msym PRIVATE -w)
endif()

if(UNIX)
  target_link_libraries(msym PUBLIC m)
endif()
