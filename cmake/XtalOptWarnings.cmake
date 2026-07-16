# This is to apply compiler warnings to XtalOpt code; and not the external/.
macro(xtalopt_enable_warnings)
  if(MSVC)
    # Ignore signed and unsigned loop comparisons in numeric loops.
    add_compile_options(/W4 /wd4018)
  else()
    # Allow partial C++11 aggregate initialization; remaining fields are zero.
    add_compile_options(-Wall -Wextra -Wno-missing-field-initializers)
  endif()
endmacro()
