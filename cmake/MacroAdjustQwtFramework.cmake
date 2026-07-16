function(xtalopt_adjust_qwt_framework target_name)
  if(NOT APPLE OR NOT QWT_LIBRARY MATCHES "\\.framework/")
    return()
  endif()

  string(REGEX REPLACE "/qwt\\.framework/.*" "" _qwt_rpath_dir "${QWT_LIBRARY}")
  get_filename_component(_qwt_real "${QWT_LIBRARY}" REALPATH)
  string(REGEX REPLACE ".*/(qwt\\.framework/.*)" "\\1" _qwt_bare_name "${_qwt_real}")

  set_property(TARGET ${target_name} APPEND PROPERTY BUILD_RPATH "${_qwt_rpath_dir}")
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND install_name_tool -change
            "${_qwt_bare_name}" "@rpath/${_qwt_bare_name}"
            "$<TARGET_FILE:${target_name}>"
    VERBATIM
    COMMENT "Rewriting Qwt load path to @rpath in build-tree binary"
  )
endfunction()
