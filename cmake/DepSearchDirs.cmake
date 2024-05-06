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
