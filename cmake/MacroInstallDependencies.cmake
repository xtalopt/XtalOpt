# Written by Patrick Avery - 2017

# Important macros:
#   InstallDependencies automatically finds dependencies for an executable
#   or shared library that aren't system libraries and installs them to a
#   specified location
#
#   CopyDependencies does the same thing except it copies the dependencies
#   instead of installing them.

# This function is only necessary because for some reason, the MSVC runtime
# DLLs are not considered system libraries in get_prerequisites(); they are
# provided by InstallRequiredSystemLibraries instead.
macro(RemoveUnneededPrereqs prereqs)
  list(REMOVE_ITEM prereqs
       "VCRUNTIME140.dll" "VCRUNTIME140_1.dll" "MSVCP140.dll")
  # Windows API-set stubs; InstallRequiredSystemLibraries redistributes these.
  list(FILTER prereqs EXCLUDE REGEX "(^|/)(api|ext)-ms-win-")
endmacro()

# This function is called automatically when a user
# calls GetPrerequisites(). This "intercepts" the file
# names and changes their type. It is most important for
# "system" files because they may be ignored by
# "GetPrerequisites" if the user specifies them to be.
function(gp_resolved_file_type_override filename type)
  if(filename MATCHES "(.*)Qt[56](.*)" OR
     filename MATCHES "(.*)libqwt(.*)" OR
     filename MATCHES "(.*)libssh(.*)" OR
     filename MATCHES "(.*)libcrypto(.*)" OR
     filename MATCHES "(.*)libgcrypt(.*)" OR
     filename MATCHES "(.*)libgpg-error(.*)" OR
     filename MATCHES "(.*)libpng(.*)" OR
     filename MATCHES "(.*)libicui18n(.*)" OR
     filename MATCHES "(.*)libicuuc(.*)" OR
     filename MATCHES "(.*)libicudata(.*)" OR
     # Qt third-party dependencies (harden the linux installation!).
     filename MATCHES "(.*)libpcre2-16(.*)" OR
     filename MATCHES "(.*)libdouble-conversion(.*)" OR
     filename MATCHES "(.*)libharfbuzz(.*)" OR
     filename MATCHES "(.*)libfreetype(.*)" OR
     filename MATCHES "(.*)libgraphite2(.*)")
    set(type "other" PARENT_SCOPE)
  endif()
endfunction()

function(_xtalopt_resolve_prerequisites ExeLocation DepSearchDirs OutputVar)
  include(GetPrerequisites)
  set(exclude_system 1)
  set(recurse 1)
  get_filename_component(exepath "${ExeLocation}" DIRECTORY)
  get_prerequisites("${ExeLocation}" prereqs "${exclude_system}"
                    "${recurse}" "${exepath}" "${DepSearchDirs}")

  RemoveUnneededPrereqs(prereqs)

  set(_resolved_prereqs)
  foreach(prereq ${prereqs})
    set(_original_prereq "${prereq}")
    foreach(dir ${DepSearchDirs})
      if(EXISTS "${dir}/${prereq}")
        set(prereq "${dir}/${prereq}")
        break()
      endif()
    endforeach()

    get_filename_component(_bare_filename "${prereq}" NAME)
    get_filename_component(_real_filename "${prereq}" REALPATH)
    list(APPEND _resolved_prereqs
         "${_original_prereq}|${_bare_filename}|${_real_filename}")
  endforeach()

  set(${OutputVar} "${_resolved_prereqs}" PARENT_SCOPE)
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

  _xtalopt_resolve_prerequisites("${ExeLocation}" "${DepSearchDirs}" _resolved_prereqs)
  foreach(prereq ${_resolved_prereqs})
    string(REPLACE "|" ";" _prereq_parts "${prereq}")
    list(GET _prereq_parts 1 _bare_filename)
    list(GET _prereq_parts 2 _real_filename)
    install(PROGRAMS "${_real_filename}"
            DESTINATION ${TargetLocation}
            RENAME ${_bare_filename})
  endforeach()
  message("-- Finished locating dependencies for ${exe_name}")
endmacro()

# Same as above but copies the dependencies instead
macro(CopyDependencies ExeLocation TargetLocation DepSearchDirs)
  get_filename_component(exe_name "${ExeLocation}" NAME)
  message("-- Locating dependencies for ${exe_name}")

  _xtalopt_resolve_prerequisites("${ExeLocation}" "${DepSearchDirs}" _resolved_prereqs)
  foreach(prereq ${_resolved_prereqs})
    string(REPLACE "|" ";" _prereq_parts "${prereq}")
    list(GET _prereq_parts 2 _real_filename)
    file(COPY "${_real_filename}"
         DESTINATION ${TargetLocation})
  endforeach()
  message("-- Finished locating dependencies for ${exe_name}")
endmacro()

# Same dependency lookup as above, but intended for install(CODE) scripts that
# need to stage files with file(INSTALL) after the executable already exists.
macro(InstallResolvedDependencies ExeLocation TargetLocation DepSearchDirs)
  get_filename_component(exe_name "${ExeLocation}" NAME)
  message(STATUS "Locating dependencies for ${exe_name}")

  _xtalopt_resolve_prerequisites("${ExeLocation}" "${DepSearchDirs}" _resolved_prereqs)
  foreach(prereq ${_resolved_prereqs})
    string(REPLACE "|" ";" _prereq_parts "${prereq}")
    list(GET _prereq_parts 1 _bare_filename)
    list(GET _prereq_parts 2 _real_filename)
    get_filename_component(_real_base_filename "${_real_filename}" NAME)
    file(INSTALL TYPE PROGRAM FILES "${_real_filename}" DESTINATION "${TargetLocation}")
    if(NOT _real_base_filename STREQUAL _bare_filename)
      file(RENAME "${TargetLocation}/${_real_base_filename}"
                  "${TargetLocation}/${_bare_filename}")
    endif()
  endforeach()
  message("-- Finished locating dependencies for ${exe_name}")
endmacro()
