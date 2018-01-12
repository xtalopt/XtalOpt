# Written by Patrick S. Avery - 2018

# Downloads the executable if it doesn't already exist
macro(DownloadGenPowderDiffraction)
  # Let's set the name. Windows likes to add '.exe' at the end
  if(WIN32)
    set(GENPOWDERDIFFRACTION_NAME "genPowderDiffraction.exe")
  else(WIN32)
    set(GENPOWDERDIFFRACTION_NAME "genPowderDiffraction")
  endif(WIN32)

  # If it already exists, don't download it again
  if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${GENPOWDERDIFFRACTION_NAME} AND NOT USE_SYSTEM_GENPOWDERDIFFRACTION)
     set(GENPOWDERDIFFRACTION_V "1.0-static")
    # Linux
    if(UNIX AND NOT APPLE)
      set(GENPOWDERDIFFRACTION_DOWNLOAD_LOCATION "https://github.com/psavery/plotXrd/releases/download/${GENPOWDERDIFFRACTION_V}/linux64-genPowderDiffraction")
      set(MD5 "e1b3c1d6b951ed83a037567490d75f1d")

    # Apple
    elseif(APPLE)
      set(GENPOWDERDIFFRACTION_DOWNLOAD_LOCATION "https://github.com/psavery/plotXrd/releases/download/${GENPOWDERDIFFRACTION_V}/osx64-genPowderDiffraction")
      set(MD5 "cdcb73c847bd147e21642f205a72e6e9")

    # Windows
    elseif(WIN32 AND NOT CYGWIN)
      set(GENPOWDERDIFFRACTION_DOWNLOAD_LOCATION "https://github.com/psavery/plotXrd/releases/download/${GENPOWDERDIFFRACTION_V}/win64-genPowderDiffraction.exe")
      set(MD5 "7b1a1e18a6044773c631189cbfd8b440")

    else()
      message(FATAL_ERROR
              "GenPowderDiffraction is not supported with the current OS type!")
    endif()

    message("-- Downloading genPowderDiffraction executable from ${GENPOWDERDIFFRACTION_DOWNLOAD_LOCATION}")

    # Install to a temporary directory so we can copy and change file
    # permissions
    file(DOWNLOAD ${GENPOWDERDIFFRACTION_DOWNLOAD_LOCATION}
         "${CMAKE_BINARY_DIR}/tmp/${GENPOWDERDIFFRACTION_NAME}"
         SHOW_PROGRESS
         EXPECTED_MD5 ${MD5})

    # We need to change the permissions
    file(COPY "${CMAKE_BINARY_DIR}/tmp/${GENPOWDERDIFFRACTION_NAME}"
         DESTINATION "${CMAKE_BINARY_DIR}/bin/"
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                          GROUP_READ GROUP_EXECUTE
                          WORLD_READ WORLD_EXECUTE)

    # Now remove the temporary directory
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/tmp")

  endif(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${GENPOWDERDIFFRACTION_NAME} AND NOT USE_SYSTEM_GENPOWDERDIFFRACTION)

  # Only install it if we are not using system genPowderDiffraction
  if(NOT USE_SYSTEM_GENPOWDERDIFFRACTION)
    set(GENPOWDERDIFFRACTION_DESTINATION "bin")
    # We need to put it in a slightly different place for apple
    if(APPLE)
      set(GENPOWDERDIFFRACTION_DESTINATION "xtalopt.app/Contents/bin")
    endif(APPLE)

    install(FILES "${CMAKE_BINARY_DIR}/bin/${GENPOWDERDIFFRACTION_NAME}"
            DESTINATION "${GENPOWDERDIFFRACTION_DESTINATION}"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_EXECUTE
                        WORLD_READ WORLD_EXECUTE)
  endif(NOT USE_SYSTEM_GENPOWDERDIFFRACTION)

endmacro(DownloadGenPowderDiffraction)
