# This function is called automatically when a user
# calls GetPrerequisites(). This "intercepts" the file
# names and changes their type. It is most important for
# "system" files because they may be ignored by
# "GetPrerequisites" if the user specifies them to be.
function(gp_resolved_file_type_override filename type)
  if(filename MATCHES "(.*)Qt5(.*)" OR
     filename MATCHES "(.*)libssh(.*)" OR
     filename MATCHES "(.*)libcrypto(.*)")
    set(type "other" PARENT_SCOPE)
  endif()
endfunction()

# Finds dependencies for the ExeLocation executable and
# then installs the dependencies to TargetLocation
# Define gp_resolved_file_type_override before calling this
# function if you wish to change the dependencies that get installed
# DepsearchDirs is a list of directories in which to search for the
# dependencies (in case they may not be found automatically)
function(InstallDependencies ExeLocation TargetLocation DepSearchDirs)
  get_filename_component(exe_name "${ExeLocation}" NAME)
  message("-- Installing dependencies for ${exe_name} to ${TargetLocation}")

  # First, get the list of dependencies
  include(GetPrerequisites)
  set(exclude_system 1)
  set(recurse 1)
  get_prerequisites("${ExeLocation}" prereq_var exclude_system
                    recurse exepath "${DepSearchDirs}")
  # Next, loop through each dependency. In case they are sym links, follow
  # the sym link to the original file, and then copy that and rename it to the
  # dependency name. E.g., install libQt5Core.so.5.5.1 and rename it to
  # libQt5Core.so.5
  foreach(loop_var ${prereq_var})
    # On Windows, loop through each DepSearchDir to try to find each .dll file
    foreach(dir ${DepSearchDirs})
      if(EXISTS "${dir}/${loop_var}")
        set(loop_var "${dir}/${loop_var}")
        break()
      endif()
    endforeach()

    get_filename_component(bareVarFilename "${loop_var}" NAME)
    get_filename_component(realfile "${loop_var}" REALPATH)
    get_filename_component(bareRealFilename "${realfile}" NAME)
    message("-- Installing ${bareVarFilename}")

    file(COPY "${realfile}" DESTINATION "${TargetLocation}"
         NO_SOURCE_PERMISSIONS)
    file(RENAME "${TargetLocation}/${bareRealFilename}"
                "${TargetLocation}/${bareVarFilename}")
  endforeach()
  message("-- Finished dependency installation for ${exe_name}")
endfunction()
