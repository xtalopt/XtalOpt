# Written by Patrick S. Avery - 2017

macro(use_cxx11)
  if(CMAKE_VERSION VERSION_LESS "3.1")
    if(UNIX OR MINGW)
      set(CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
    # For MSVC, we must have VS 2013 or greater
    elseif(MSVC)
      if(MSVC_VERSION LESS 1800)
        message(FATAL_ERROR "MSVC must be VS 2013 or greater")
      endif()
    endif()
  else()
    set(CMAKE_CXX_STANDARD 11)
  endif()
endmacro(use_cxx11)
