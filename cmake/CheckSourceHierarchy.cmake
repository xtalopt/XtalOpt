# Check source-layer dependencies.
# This is a light-weight and CMake-only script, added to enforce
#   the source-layer dependency rules.

if(NOT DEFINED XTALOPT_SOURCE_DIR)
  message(FATAL_ERROR "XTALOPT_SOURCE_DIR is required")
endif()

set(_hierarchy_failures)

function(_xoh_check_file file module_name banned_include_regex
         banned_namespace_regex)
  file(STRINGS "${file}" _lines)
  set(_line_number 0)
  foreach(_line IN LISTS _lines)
    math(EXPR _line_number "${_line_number} + 1")

    if(banned_include_regex AND
       _line MATCHES "^[ \t]*#[ \t]*include[ \t]*[<\"].*(${banned_include_regex})/")
      list(APPEND _hierarchy_failures
           "${module_name}: forbidden include in ${file}:${_line_number}: ${_line}")
    endif()

    if(banned_namespace_regex AND _line MATCHES "${banned_namespace_regex}")
      list(APPEND _hierarchy_failures
           "${module_name}: forbidden namespace reference in ${file}:${_line_number}: ${_line}")
    endif()
  endforeach()

  set(_hierarchy_failures "${_hierarchy_failures}" PARENT_SCOPE)
endfunction()

function(_xoh_check_tree module_name relative_dir banned_include_regex
         banned_namespace_regex)
  # A missing source tree is an error, not an empty check!
  if(NOT IS_DIRECTORY "${XTALOPT_SOURCE_DIR}/${relative_dir}")
    message(FATAL_ERROR
            "Source hierarchy check is misconfigured: '${relative_dir}' does "
            "not exist. Update cmake/CheckSourceHierarchy.cmake after a "
            "source-tree rename.")
  endif()

  file(GLOB_RECURSE _source_files
       "${XTALOPT_SOURCE_DIR}/${relative_dir}/*.h"
       "${XTALOPT_SOURCE_DIR}/${relative_dir}/*.hpp"
       "${XTALOPT_SOURCE_DIR}/${relative_dir}/*.cpp"
       "${XTALOPT_SOURCE_DIR}/${relative_dir}/*.cxx"
       "${XTALOPT_SOURCE_DIR}/${relative_dir}/*.cc")

  foreach(_file IN LISTS _source_files)
    _xoh_check_file("${_file}" "${module_name}" "${banned_include_regex}"
                    "${banned_namespace_regex}")
  endforeach()

  set(_hierarchy_failures "${_hierarchy_failures}" PARENT_SCOPE)
endfunction()

# common must not depend on any other project module.
_xoh_check_tree("src/common"
                "src/common"
                "atoms|generatexrd|search|xtalopt"
                "Atoms::|GenerateXrd::|Search::|XtalOpt::")

# atoms may depend only on common and atoms.
_xoh_check_tree("src/atoms"
                "src/atoms"
                "generatexrd|search|xtalopt"
                "GenerateXrd::|Search::|XtalOpt::")

# generatexrd may depend only on common and atoms.
_xoh_check_tree("src/generatexrd"
                "src/generatexrd"
                "search|xtalopt"
                "Search::|XtalOpt::")

# search may depend only on common and atoms.
_xoh_check_tree("src/search"
                "src/search"
                "generatexrd|xtalopt"
                "GenerateXrd::|XtalOpt::")

if(_hierarchy_failures)
  string(REPLACE ";" "\n" _hierarchy_message "${_hierarchy_failures}")
  message(FATAL_ERROR
          "Source hierarchy check failed:\n${_hierarchy_message}")
endif()

message(STATUS "Source hierarchy check passed")
