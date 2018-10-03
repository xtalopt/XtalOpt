# Written by Patrick Avery - 2017

# Important macros:
#   InstallDependencies automatically finds dependencies for an executable
#   or shared library that aren't system libraries and installs them to a
#   specified location
#
#   CopyDependencies does the same thing except it copies the dependencies
#   instead of installing them.

# This function is only necessary because for some reason, VCRUNTIME140.dll is not
# considered a system library in get_prerequisites()
macro(RemoveUnneededPrereqs prereqs)
  list(REMOVE_ITEM prereqs "VCRUNTIME140.dll")
endmacro()

# This function is called automatically when a user
# calls GetPrerequisites(). This "intercepts" the file
# names and changes their type. It is most important for
# "system" files because they may be ignored by
# "GetPrerequisites" if the user specifies them to be.
function(gp_resolved_file_type_override filename type)
  if(filename MATCHES "(.*)Qt5(.*)" OR
     filename MATCHES "(.*)libssh(.*)" OR
     filename MATCHES "(.*)libcrypto(.*)" OR
     filename MATCHES "(.*)libpng(.*)" OR
     filename MATCHES "(.*)libicui18n(.*)" OR
     filename MATCHES "(.*)libicuuc(.*)" OR
     filename MATCHES "(.*)libicudata(.*)")
    set(type "other" PARENT_SCOPE)
  endif()
endfunction()

# Finds dependencies for the ExeLocation executable and
# then installs the dependencies to TargetLocation
# Define gp_resolved_file_type_override before calling this
# function if you wish to change the dependencies that get installed
# DepsearchDirs is a list of directories in which to search for the
# dependencies (in case they may not be found automatically)
macro(InstallDependencies ExeLocation TargetLocation DepSearchDirs)
  get_filename_component(exe_name "${ExeLocation}" NAME)
  message("-- Locating dependencies for ${exe_name}")

  # First, get the list of dependencies
  include(GetPrerequisites)
  set(exclude_system 1)
  set(recurse 1)
  get_prerequisites("${ExeLocation}" prereqs exclude_system
                    recurse exepath "${DepSearchDirs}")

  # For some reason, some system libraries are still sometimes included.
  # Remove them if they are included.
  RemoveUnneededPrereqs(prereqs)

  # Next, loop through each dependency. In case they are sym links, follow
  # the sym link to the original file, and then install that and rename it to
  # the dependency name. E.g., install libQt5Core.so.5.5.1 and rename it to
  # libQt5Core.so.5
  foreach(prereq ${prereqs})
    # First, loop through each DepSearchDir to try to see if we can find it
    foreach(dir ${DepSearchDirs})
      if(EXISTS "${dir}/${prereq}")
        # Set the absolute file path if we do
        set(prereq "${dir}/${prereq}")
        break()
      endif()
    endforeach()

    get_filename_component(bareVarFilename "${prereq}" NAME)
    get_filename_component(realfile "${prereq}" REALPATH)
    get_filename_component(bareRealFilename "${realfile}" NAME)

    install(FILES "${realfile}"
            DESTINATION ${TargetLocation}
            RENAME ${bareVarFilename})
  endforeach()
  message("-- Finished locating dependencies for ${exe_name}")
endmacro()

# Same as above but copies the dependencies instead
macro(CopyDependencies ExeLocation TargetLocation DepSearchDirs)
  get_filename_component(exe_name "${ExeLocation}" NAME)
  message("-- Locating dependencies for ${exe_name}")

  # First, get the list of dependencies
  include(GetPrerequisites)
  set(exclude_system 1)
  set(recurse 1)
  get_prerequisites("${ExeLocation}" prereqs exclude_system
                    recurse exepath "${DepSearchDirs}")

  # For some reason, some system libraries are still sometimes included.
  # Remove them if they are included.
  RemoveUnneededPrereqs(prereqs)

  # Next, loop through each dependency. In case they are sym links, follow
  # the sym link to the original file, and then install that and rename it to
  # the dependency name. E.g., install libQt5Core.so.5.5.1 and rename it to
  # libQt5Core.so.5
  foreach(prereq ${prereqs})
    # First, loop through each DepSearchDir to try to see if we can find it
    foreach(dir ${DepSearchDirs})
      if(EXISTS "${dir}/${prereq}")
        # Set the absolute file path if we do
        set(prereq "${dir}/${prereq}")
        break()
      endif()
    endforeach()

    get_filename_component(bareVarFilename "${prereq}" NAME)
    get_filename_component(realfile "${prereq}" REALPATH)
    get_filename_component(bareRealFilename "${realfile}" NAME)

    file(COPY "${realfile}"
         DESTINATION ${TargetLocation})
  endforeach()
  message("-- Finished locating dependencies for ${exe_name}")
endmacro()
