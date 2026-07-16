if(NOT DEFINED XTALOPT_RUNTIME_DEPS_MODULE_PATH OR
   NOT DEFINED XTALOPT_RUNTIME_DEPS_EXE OR
   NOT DEFINED XTALOPT_RUNTIME_DEPS_DESTINATION OR
   NOT DEFINED XTALOPT_RUNTIME_DEPS_SEARCH_DIRS)
  message(FATAL_ERROR
          "CopyRuntimeDependencies.cmake requires module path, executable, "
          "destination, and search directories.")
endif()

list(APPEND CMAKE_MODULE_PATH "${XTALOPT_RUNTIME_DEPS_MODULE_PATH}")
include(MacroInstallDependencies)

CopyDependencies("${XTALOPT_RUNTIME_DEPS_EXE}"
                 "${XTALOPT_RUNTIME_DEPS_DESTINATION}"
                 "${XTALOPT_RUNTIME_DEPS_SEARCH_DIRS}")
