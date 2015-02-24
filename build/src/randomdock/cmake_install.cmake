# Install script for directory: /Users/Zeek/src/xtalopt-release9/src/randomdock

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/Zeek")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib/randomdock.dylib")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib" TYPE SHARED_LIBRARY FILES "/Users/Zeek/src/xtalopt-release9/build/src/randomdock/randomdock.dylib")
  if(EXISTS "$ENV{DESTDIR}/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib/randomdock.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib/randomdock.dylib")
    execute_process(COMMAND "/opt/local/bin/install_name_tool"
      -id "randomdock.dylib"
      "$ENV{DESTDIR}/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib/randomdock.dylib")
    execute_process(COMMAND /opt/local/bin/install_name_tool
      -delete_rpath "/Applications/Avogadro.app/Contents/lib"
      "$ENV{DESTDIR}/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib/randomdock.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/Applications/Avogadro.app/Contents/lib/avogadro/1_1/contrib/randomdock.dylib")
    endif()
  endif()
endif()

