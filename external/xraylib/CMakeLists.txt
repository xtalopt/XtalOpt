set(xraylib_subset_SRCS
  scattering.c
  splint.c
  xraylib-error.c
  xraylib-aux.c
  xrayglob_rayl.c
)

add_library(xraylib STATIC ${xraylib_subset_SRCS})
# SYSTEM so consumers in src/ get this as a system include and the project
#   warning flags stay quiet inside the vendored header.
target_include_directories(xraylib SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(xraylib PRIVATE _GNU_SOURCE)
if(NOT WIN32)
  target_link_libraries(xraylib PRIVATE m)
endif()
