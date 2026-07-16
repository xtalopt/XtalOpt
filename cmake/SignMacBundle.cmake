# Set codesign arguments.
# "Ad-hoc" signing cann't have a secure timestamp; but a
#   developer ID signing needs a timestamp and runtime.
function(_xtalopt_codesign_extra_args _identity _out_var)
  if(_identity STREQUAL "-")
    set(${_out_var} --timestamp=none PARENT_SCOPE)
  else()
    set(${_out_var} --timestamp --options runtime PARENT_SCOPE)
  endif()
endfunction()

function(_xtalopt_codesign_path _path _identity _codesign_executable)
  _xtalopt_codesign_extra_args("${_identity}" _xtalopt_codesign_extra)
  execute_process(
    COMMAND "${_codesign_executable}"
            --force
            --sign "${_identity}"
            ${_xtalopt_codesign_extra}
            "${_path}"
    RESULT_VARIABLE _xtalopt_codesign_result
    OUTPUT_VARIABLE _xtalopt_codesign_output
    ERROR_VARIABLE _xtalopt_codesign_error
  )
  if(NOT _xtalopt_codesign_result EQUAL 0)
    message(FATAL_ERROR
            "codesign failed for ${_path}\n"
            "${_xtalopt_codesign_output}\n${_xtalopt_codesign_error}")
  endif()
endfunction()

function(xtalopt_sign_macos_bundle _bundle_path _identity _codesign_executable)
  if(NOT EXISTS "${_bundle_path}")
    message(FATAL_ERROR "Cannot sign missing bundle: ${_bundle_path}")
  endif()

  # Sign the bundle before signing its nested files.
  _xtalopt_codesign_extra_args("${_identity}" _xtalopt_codesign_extra)
  execute_process(
    COMMAND "${_codesign_executable}"
            --force
            --deep
            --sign "${_identity}"
            ${_xtalopt_codesign_extra}
            "${_bundle_path}"
    RESULT_VARIABLE _xtalopt_bundle_sign_result
    OUTPUT_VARIABLE _xtalopt_bundle_sign_output
    ERROR_VARIABLE _xtalopt_bundle_sign_error
  )
  if(NOT _xtalopt_bundle_sign_result EQUAL 0)
    message(FATAL_ERROR
            "Initial bundle codesign failed for ${_bundle_path}\n"
            "${_xtalopt_bundle_sign_output}\n${_xtalopt_bundle_sign_error}")
  endif()

  # Do not follow bundle links while looking for files to sign. Sign each file once.
  # Keep this policy change inside this function.
  cmake_policy(PUSH)
  cmake_policy(SET CMP0009 NEW)
  file(GLOB_RECURSE _xtalopt_nested_dylibs
       LIST_DIRECTORIES FALSE
       "${_bundle_path}/*.dylib"
       "${_bundle_path}/*.so")
  cmake_policy(POP)
  foreach(_path IN LISTS _xtalopt_nested_dylibs)
    _xtalopt_codesign_path("${_path}" "${_identity}" "${_codesign_executable}")
  endforeach()

  file(GLOB _xtalopt_helper_binaries
       LIST_DIRECTORIES FALSE
       "${_bundle_path}/Contents/MacOS/*"
       "${_bundle_path}/Contents/bin/*")
  foreach(_path IN LISTS _xtalopt_helper_binaries)
    if(NOT IS_DIRECTORY "${_path}")
      _xtalopt_codesign_path("${_path}" "${_identity}" "${_codesign_executable}")
    endif()
  endforeach()

  file(GLOB _xtalopt_framework_bundles
       LIST_DIRECTORIES TRUE
       "${_bundle_path}/Contents/Frameworks/*.framework")
  foreach(_framework IN LISTS _xtalopt_framework_bundles)
    if(IS_DIRECTORY "${_framework}")
      _xtalopt_codesign_path("${_framework}" "${_identity}" "${_codesign_executable}")
    endif()
  endforeach()

  # Sign the final bundle.
  _xtalopt_codesign_path("${_bundle_path}" "${_identity}" "${_codesign_executable}")

  execute_process(
    COMMAND "${_codesign_executable}"
            --verify
            --deep
            --strict
            "${_bundle_path}"
    RESULT_VARIABLE _xtalopt_verify_result
    OUTPUT_VARIABLE _xtalopt_verify_output
    ERROR_VARIABLE _xtalopt_verify_error
  )
  if(NOT _xtalopt_verify_result EQUAL 0)
    message(FATAL_ERROR
            "codesign verification failed for ${_bundle_path}\n"
            "${_xtalopt_verify_output}\n${_xtalopt_verify_error}")
  endif()
endfunction()

function(xtalopt_sign_macos_cli_install_tree _exe_path _runtime_dir _identity _codesign_executable)
  if(NOT EXISTS "${_exe_path}")
    message(FATAL_ERROR "Cannot sign missing executable: ${_exe_path}")
  endif()

  if(EXISTS "${_runtime_dir}")
    file(GLOB _xtalopt_cli_runtime_items
         LIST_DIRECTORIES TRUE
         "${_runtime_dir}/*")
    foreach(_path IN LISTS _xtalopt_cli_runtime_items)
      if(IS_DIRECTORY "${_path}")
        if(_path MATCHES "\\.framework$")
          get_filename_component(_framework_name "${_path}" NAME_WE)
          file(GLOB _framework_binaries
               LIST_DIRECTORIES FALSE
               "${_path}/${_framework_name}"
               "${_path}/Versions/*/${_framework_name}")
          foreach(_framework_binary IN LISTS _framework_binaries)
            get_filename_component(_framework_binary_real
                                   "${_framework_binary}" REALPATH)
            _xtalopt_codesign_path("${_framework_binary_real}"
                                   "${_identity}"
                                   "${_codesign_executable}")
          endforeach()
        endif()
      else()
        _xtalopt_codesign_path("${_path}" "${_identity}" "${_codesign_executable}")
      endif()
    endforeach()
  endif()

  _xtalopt_codesign_path("${_exe_path}" "${_identity}" "${_codesign_executable}")

  execute_process(
    COMMAND "${_codesign_executable}"
            --verify
            --deep
            --strict
            "${_exe_path}"
    RESULT_VARIABLE _xtalopt_verify_result
    OUTPUT_VARIABLE _xtalopt_verify_output
    ERROR_VARIABLE _xtalopt_verify_error
  )
  if(NOT _xtalopt_verify_result EQUAL 0)
    message(FATAL_ERROR
            "codesign verification failed for ${_exe_path}\n"
            "${_xtalopt_verify_output}\n${_xtalopt_verify_error}")
  endif()
endfunction()
