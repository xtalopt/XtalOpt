set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/modules/
)

include(DefineCompilerFlags)
include(DefineOptions.cmake)

# These aren't configurable for the XtalOpt build
set (WITH_STATIC_LIB ON)
set (WITH_SFTP ON)
set (WITH_SERVER OFF)

# search for libraries
if (WITH_LIBZ)
    find_package(ZLIB REQUIRED)
endif (WITH_LIBZ)

if (WITH_GCRYPT)
  find_package(GCrypt REQUIRED)
  if (NOT GCRYPT_FOUND)
    message(FATAL_ERROR "Could not find GCrypt")
  endif (NOT GCRYPT_FOUND)
else (WITH_GCRYPT)
  find_package(OpenSSL)
  if (NOT OPENSSL_FOUND)
    find_package(GCrypt)
    if (NOT GCRYPT_FOUND)
      message(FATAL_ERROR "Could not find OpenSSL or GCrypt")
    endif (NOT GCRYPT_FOUND)
  endif (NOT OPENSSL_FOUND)
endif(WITH_GCRYPT)

# config.h checks
include(ConfigureChecks.cmake)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

INCLUDE_DIRECTORIES( ${XtalOpt_BINARY_DIR}/src/libssh/
		     ${XtalOpt_SOURCE_DIR}/src/libssh/
		     ${CMAKE_CURRENT_BINARY_DIR}/
		     )


set(LIBSSH_PUBLIC_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/src/libssh/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src/libssh/
  CACHE INTERNAL "libssh public include directories"
)

set(LIBSSH_PRIVATE_INCLUDE_DIRS
  ${CMAKE_BINARY_DIR}
  ${ZLIB_INCLUDE_DIRS}
)

if (WITH_STATIC_LIB)
  set(LIBSSH_STATIC_LIBRARY
    ssh_static
    CACHE INTERNAL "libssh static library"
  )
endif (WITH_STATIC_LIB)

set(LIBSSH_LINK_LIBRARIES
  ${LIBSSH_REQUIRED_LIBRARIES}
  ${ZLIB_LIBRARIES}
)

if (WIN32)
  set(LIBSSH_LINK_LIBRARIES
    ${LIBSSH_LINK_LIBRARIES}
    ws2_32
  )
endif (WIN32)

if (HAVE_LIBSOCKET)
  set(LIBSSH_LINK_LIBRARIES
    ${LIBSSH_LINK_LIBRARIES}
    socket
  )
endif (HAVE_LIBSOCKET)

if (OPENSSL_LIBRARIES)
  set(LIBSSH_PRIVATE_INCLUDE_DIRS
    ${LIBSSH_PRIVATE_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
  )

  set(LIBSSH_LINK_LIBRARIES
    ${LIBSSH_LINK_LIBRARIES}
    ${OPENSSL_LIBRARIES}
  )
endif (OPENSSL_LIBRARIES)

if (GCRYPT_LIBRARY)
  set(LIBSSH_PRIVATE_INCLUDE_DIRS
    ${LIBSSH_PRIVATE_INCLUDE_DIRS}
    ${GCRYPT_INCLUDE_DIRS}
  )

  set(LIBSSH_LINK_LIBRARIES
    ${LIBSSH_LINK_LIBRARIES}
    ${GCRYPT_LIBRARY}
  )
endif (GCRYPT_LIBRARY)

set(LIBSSH_LINK_LIBRARIES
  ${LIBSSH_LINK_LIBRARIES}
  CACHE INTERNAL "libssh link libraries"
)

set(libssh_SRCS
  agent.c
  auth1.c
  auth.c
  base64.c
  buffer.c
  callbacks.c
  channels1.c
  channels.c
  client.c
  config.c
  connect.c
  crc32.c
  crypt.c
  dh.c
  error.c
  gcrypt_missing.c
  gzip.c
  init.c
  kex.c
  keyfiles.c
  keys.c
  log.c
  match.c
  messages.c
  misc.c
  options.c
  packet.c
  pcap.c
  poll.c
  scp.c
  server.c
  session.c
  sftp.c
  sftpserver.c
  socket.c
  string.c
  wrapper.c
)

if (WITH_SFTP)
  set(libssh_SRCS
    ${libssh_SRCS}
    sftp.c
  )

  if (WITH_SERVER)
    set(libssh_SRCS
      ${libssh_SRCS}
      sftpserver.c
    )
  endif (WITH_SERVER)
endif (WITH_SFTP)

if (WITH_SSH1)
  set(libssh_SRCS
    ${libssh_SRCS}
    auth1.c
    channels1.c
    packet1.c
  )
endif (WITH_SSH1)

if (WITH_SERVER)
  set(libssh_SRCS
    ${libssh_SRCS}
    server.c
  )
endif (WITH_SERVER)

include_directories(
  ${LIBSSH_PUBLIC_INCLUDE_DIRS}
  ${LIBSSH_PRIVATE_INCLUDE_DIRS}
)

install(
  TARGETS
    ${LIBSSH_SHARED_LIBRARY}
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  COMPONENT libraries
)

if (WITH_STATIC_LIB)
  add_library(${LIBSSH_STATIC_LIBRARY} STATIC ${libssh_SRCS})

  set_target_properties(
    ${LIBSSH_STATIC_LIBRARY}
      PROPERTIES
        VERSION
          ${LIBRARY_VERSION}
        SOVERSION
          ${LIBRARY_SOVERSION}
        COMPILE_FLAGS
          "-DLIBSSH_STATIC"
  )

# No installation!
endif (WITH_STATIC_LIB)

