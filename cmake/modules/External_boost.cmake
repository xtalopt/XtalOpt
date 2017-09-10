# This will install boost and set the variables
# BOOST_ROOT, Boost_INCLUDE_DIR, and Boost_LIBRARY_DIR
# To their respective locations

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(am 64)
else()
  set(am 32)
endif()

set(boost_with_args
  --with-regex
  --with-system
  --with-thread
)

if(WIN32)
  # We only support MSVC 2015 and 2017 currently, both of which can
  # use the msvc-14.0 toolset. So we will only use that toolset
  set(_toolset "msvc-14.0")

  list(APPEND boost_with_args
    "--layout=tagged" "toolset=${_toolset}")

  set(boost_cmds
    CONFIGURE_COMMAND bootstrap.bat
    BUILD_COMMAND b2 address-model=${am} ${boost_with_args}
    INSTALL_COMMAND b2 address-model=${am} ${boost_with_args}
      --prefix=<INSTALL_DIR> install
  )
else()
  set(boost_cmds
    CONFIGURE_COMMAND ./bootstrap.sh --prefix=<INSTALL_DIR>
    BUILD_COMMAND ./b2 address-model=${am} ${boost_with_args}
    INSTALL_COMMAND ./b2 address-model=${am} ${boost_with_args}
      install
  )
endif()

set(_v 62)
set(boost_version 1.${_v}.0)
set(boost_url "http://sourceforge.net/projects/boost/files/boost/1.${_v}.0/boost_1_${_v}_0.tar.gz/download")
set(boost_md5 "6f4571e7c5a66ccc3323da6c24be8f05")

ExternalProject_Add(boost
  DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/downloads"
  URL ${boost_url}
  URL_MD5 ${boost_md5}
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/boost/"
  INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/boost-prefix"
  ${boost_cmds}
  BUILD_IN_SOURCE 1
)

ExternalProject_Get_Property(boost install_dir)
set(BOOST_ROOT "${install_dir}" CACHE INTERNAL "")

set(Boost_INCLUDE_DIRS "${BOOST_ROOT}/include" PARENT_SCOPE)
set(Boost_LIBRARY_DIRS "${BOOST_ROOT}/lib" PARENT_SCOPE)
