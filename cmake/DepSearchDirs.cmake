# The lib dir itself (Unix layouts) plus ../bin (Windows DLL layout).
if(ENABLE_LIBSSH AND LIBSSH_LIBRARIES)
  get_filename_component(LIBSSH_DLL_DIR "${LIBSSH_LIBRARIES}" DIRECTORY)
  list(APPEND DEP_SEARCH_DIRS "${LIBSSH_DLL_DIR}" "${LIBSSH_DLL_DIR}/../bin")
endif()

# Qwt library directory and the usual Windows DLL directory.
if(QWT_LIBRARIES)
  get_filename_component(QWT_DLL_DIR "${QWT_LIBRARIES}" DIRECTORY)
  list(APPEND DEP_SEARCH_DIRS "${QWT_DLL_DIR}" "${QWT_DLL_DIR}/../bin")
endif()
if(QWT_RUNTIME_LIBRARY)
  get_filename_component(QWT_RUNTIME_DIR "${QWT_RUNTIME_LIBRARY}" DIRECTORY)
  list(APPEND DEP_SEARCH_DIRS "${QWT_RUNTIME_DIR}")
endif()

# All of the Qt dependencies will hopefully be together in the bin of the
# root directory. We will need to change this part in the future if they
# are not.
get_target_property(QtCore_location Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_RELEASE)
if(NOT QtCore_location)
  get_target_property(QtCore_location Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION)
endif()
if(NOT QtCore_location)
  get_target_property(QtCore_location Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_DEBUG)
endif()
get_filename_component(QtCore_location "${QtCore_location}" DIRECTORY)
# For a framework Qt this yields .../QtCore.framework/Versions/A; strip
#   back to the frameworks root directory.
string(REGEX REPLACE "/Qt[A-Za-z0-9]+\\.framework/.*" "" QtCore_location "${QtCore_location}")
list(APPEND DEP_SEARCH_DIRS "${QtCore_location}")
