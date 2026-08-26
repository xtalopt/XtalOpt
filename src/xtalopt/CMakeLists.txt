set(xtalopt_core_SRCS
    xtalopt.h
    genetic.h
    legacy/input_compat.h
    legacy/input_compat.cpp
    legacy/legacy_helpers.h
    legacy/legacy_helpers.cpp
    legacy/state_compat.h
    legacy/state_compat.cpp
    legacy/structure_state_compat.cpp
    legacy/queue_command_compat.cpp
    xtalopt.cpp
    structuregen.cpp
    io_settings.cpp
    io_state.cpp
    io_results.cpp
    evaluate.cpp
    genetic.cpp
    settings.cpp
    structures/xtal.cpp)

set(xtalopt_gui_SRCS
    gui/dialog.cpp
    gui/randspg_dialog.cpp
    gui/tab_struc.cpp
    gui/tab_opt.cpp
    gui/tab_search.cpp
    gui/tab_mo.cpp
    gui/tab_progress.cpp
    gui/tab_plot.cpp
    gui/xtalopt_plot.cpp
    gui/tab_log.cpp
    gui/tab_about.cpp)

# Use system includes for third-party headers (excluded from our warning handling).
set(_xtalopt_external_include_dirs ${CMAKE_SOURCE_DIR}/external)

add_library(xtalopt_core STATIC ${xtalopt_core_SRCS})

target_link_libraries(xtalopt_core
  PUBLIC
    search_core
  PRIVATE
    common_math)
target_include_directories(xtalopt_core SYSTEM PRIVATE ${_xtalopt_external_include_dirs})

if(BUILD_XTALOPT_GUI)
  add_library(xtalopt_gui STATIC ${xtalopt_gui_SRCS})
  target_link_libraries(xtalopt_gui
    PUBLIC
      xtalopt_core
      search_gui
      atoms_gui
      generatexrd_gui)
  target_include_directories(xtalopt_gui SYSTEM PRIVATE
    ${_xtalopt_external_include_dirs})
endif()

add_library(structuretool STATIC
    structuretool.h
    structuretool.cpp)
target_link_libraries(structuretool
  PUBLIC
    atoms_core
  PRIVATE
    args)
target_include_directories(structuretool SYSTEM PRIVATE
  ${_xtalopt_external_include_dirs})

# Numeric patch level for Windows and macOS version fields.
string(REGEX MATCH "^[0-9]+" _xtalopt_patch_numeric "${XtalOpt_VERSION_PATCH}")

set(xtalopt_BUNDLE_RESOURCES)
set(_xtalopt_executable_sources
    main.cpp)

if(BUILD_XTALOPT_GUI)
  list(APPEND _xtalopt_executable_sources xtalopt.qrc)
endif()

if(WIN32 AND BUILD_XTALOPT_GUI)
  # Make the Windows icon and version file.
  set(XTALOPT_RC_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/images/xtalopt-logo.ico")
  set(XTALOPT_RC_FILEVERSION
      "${XtalOpt_VERSION_MAJOR},${XtalOpt_VERSION_MINOR},${_xtalopt_patch_numeric},0")
  configure_file(xtalopt.rc.in "${CMAKE_CURRENT_BINARY_DIR}/xtalopt.rc" @ONLY)
  list(APPEND _xtalopt_executable_sources "${CMAKE_CURRENT_BINARY_DIR}/xtalopt.rc")
endif()

if(APPLE AND BUILD_XTALOPT_GUI)
  list(APPEND xtalopt_BUNDLE_RESOURCES images/xtalopt-logo.icns)
  set_source_files_properties(${xtalopt_BUNDLE_RESOURCES}
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  add_executable(xtalopt MACOSX_BUNDLE ${_xtalopt_executable_sources}
                 ${xtalopt_BUNDLE_RESOURCES})
elseif(WIN32 AND BUILD_XTALOPT_GUI)
  add_executable(xtalopt WIN32 ${_xtalopt_executable_sources})
else()
  add_executable(xtalopt ${_xtalopt_executable_sources})
endif()

target_link_libraries(xtalopt
  PRIVATE
    xtalopt_core
    structuretool
    search_cli
    args)
target_include_directories(xtalopt SYSTEM PRIVATE
  ${_xtalopt_external_include_dirs})

if(BUILD_XTALOPT_GUI)
  target_link_libraries(xtalopt PRIVATE xtalopt_gui)
endif()

if(UNIX AND ENABLE_RPATH)
  if(APPLE AND BUILD_XTALOPT_GUI)
    set(_xtalopt_install_rpath "@loader_path/../Frameworks")
  elseif(APPLE)
    set(_xtalopt_install_rpath "@loader_path/../lib")
  else()
    set(_xtalopt_install_rpath "$ORIGIN/../lib")
  endif()
  set_target_properties(xtalopt PROPERTIES
    INSTALL_RPATH "${_xtalopt_install_rpath}"
    SKIP_BUILD_RPATH FALSE
    BUILD_WITH_INSTALL_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH FALSE
  )
endif()

if(WIN32)
  # Windows finds DLLs beside the executable. Copy them for build and test runs.
  # Use one script per build configuration for multi-configuration generators.
  set(_copy_xtalopt_deps_script
      "${CMAKE_CURRENT_BINARY_DIR}/copy_xtalopt_runtime_dependencies_$<CONFIG>.cmake")
  file(GENERATE OUTPUT "${_copy_xtalopt_deps_script}" CONTENT
"set(XTALOPT_RUNTIME_DEPS_MODULE_PATH [==[${CMAKE_SOURCE_DIR}/cmake]==])
set(XTALOPT_RUNTIME_DEPS_EXE [==[$<TARGET_FILE:xtalopt>]==])
set(XTALOPT_RUNTIME_DEPS_DESTINATION [==[$<TARGET_FILE_DIR:xtalopt>]==])
set(XTALOPT_RUNTIME_DEPS_SEARCH_DIRS [==[${DEP_SEARCH_DIRS}]==])
include([==[${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake]==])
")
  add_custom_command(TARGET xtalopt POST_BUILD
                     COMMAND "${CMAKE_COMMAND}" -P "${_copy_xtalopt_deps_script}"
                     VERBATIM)
endif()

if(APPLE AND BUILD_XTALOPT_GUI)
  # CFBundleVersion needs a numeric value. Keep the full version in the long field.
  set_target_properties(xtalopt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "XtalOpt"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.xtalopt.xtalopt"
    MACOSX_BUNDLE_BUNDLE_VERSION "${XtalOpt_VERSION}.${_xtalopt_patch_numeric}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${XtalOpt_VERSION}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${XtalOpt_VERSION_FULL}"
    MACOSX_BUNDLE_ICON_FILE "xtalopt-logo.icns"
  )
endif()

if(BUILD_XTALOPT_GUI)
  include(MacroAdjustQwtFramework)
  xtalopt_adjust_qwt_framework(xtalopt)
endif()

if(BUILD_TESTS)
  set(_structuretool_sample
      "${CMAKE_SOURCE_DIR}/tests/sampledata/formats/diamond-primitive.POSCAR")
  add_test(NAME structuretool_help
           COMMAND xtalopt --input "${_structuretool_sample}" --volume --help)
  add_test(NAME structuretool_primitive
           COMMAND xtalopt --input "${_structuretool_sample}" --primitive)
  add_test(NAME structuretool_conventional
           COMMAND xtalopt --input "${_structuretool_sample}" --conventional)
  add_test(NAME structuretool_info
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --symmetry --volume --lattice --shortest-distance --formula)
  add_test(NAME structuretool_transform_output
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --niggli --standard-orient --rescale-volume 100
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-test.POSCAR"
                   --output-format VASP)
  add_test(NAME structuretool_cml_output
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-test.cml"
                   --output-format CML)
  add_test(NAME structuretool_cif_output
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-test.cif"
                   --output-format CIF)
  add_test(NAME structuretool_xyz_output
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-test.xyz"
                   --output-format XYZ)
  add_test(NAME structuretool_mtp_output
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-test.cfg"
                   --output-format MTP)
  add_test(NAME structuretool_molcrystal_output
           COMMAND xtalopt --molcrystal 19
                   "C 1.20 2.10 0.80, H 0.70 1.60 0.40"
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-molcrystal-test.POSCAR"
                   --output-format VASP)
  add_test(NAME structuretool_molunit_xyz_output
           COMMAND xtalopt --molunit H 2 linear_2_pair
                   --output "${CMAKE_CURRENT_BINARY_DIR}/structuretool-molunit-test.xyz")
  add_test(NAME structuretool_rdf
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --rdf --rdf-bins 5 --rdf-cutoff 6 --rdf-sigma 0.2)
  add_test(NAME structuretool_compare_rdf
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --compare-rdf "${_structuretool_sample}"
                   --rdf-bins 5 --rdf-cutoff 6 --rdf-sigma 0.2)
  add_test(NAME structuretool_compare_xtalcomp
           COMMAND xtalopt --input "${_structuretool_sample}"
                   --compare-xtalcomp "${_structuretool_sample}")
  set_tests_properties(structuretool_help
                       structuretool_primitive
                       structuretool_conventional
                       structuretool_info
                       structuretool_transform_output
                       structuretool_cml_output
                       structuretool_cif_output
                       structuretool_xyz_output
                       structuretool_mtp_output
                       structuretool_molcrystal_output
                       structuretool_molunit_xyz_output
                       structuretool_rdf
                       structuretool_compare_rdf
                       structuretool_compare_xtalcomp
                       PROPERTIES LABELS "search;structuretool")
endif()
# Install instructions
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(UNIX)
    # Use a versioned directory in the user's home directory.
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/xtalopt-${XtalOpt_VERSION_FULL}"
        CACHE PATH "default install path" FORCE)
  elseif(WIN32)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/XtalOpt" CACHE PATH
        "default install path" FORCE)
  endif()
endif()

install(TARGETS xtalopt
  BUNDLE DESTINATION .
  RUNTIME DESTINATION bin
)

# Install documents and schemes outside the macOS bundle. Loose bundle files
# lose their signature when a ZIP file is unpacked.
install(FILES
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/ChangeLog
        ${CMAKE_SOURCE_DIR}/external/THIRD_PARTY_LICENSES.txt
        DESTINATION ".")
install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/schemes
        DESTINATION ".")

if(UNIX AND NOT APPLE)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/xtalopt-logo.png"
          DESTINATION ".")
  if(BUILD_XTALOPT_GUI)
    # Install a launcher in the package root in linux. The user may copy it to an
    # applications directory without replacing an existing launcher.
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/images/xtalopt.desktop.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/xtalopt.desktop" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xtalopt.desktop"
            DESTINATION ".")
  endif()
endif()

if(APPLE AND BUILD_XTALOPT_GUI)
  set(_qt_plugin_install_root "xtalopt.app/Contents/plugins")

  include(MacroInstallCocoa)
  InstallCocoa("${_qt_plugin_install_root}" QT_PLUGINS)

  if(TARGET "Qt${QT_VERSION_MAJOR}::QMacStylePlugin")
    include(MacroInstallStyle)
    InstallQMacStyle("${_qt_plugin_install_root}" QT_PLUGINS)
  endif()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
       "[Paths]\nPlugins = plugins\n")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
          DESTINATION "xtalopt.app/Contents/Resources")

  get_target_property(_qtcore_loc Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION_RELEASE)
  if(NOT _qtcore_loc)
    get_target_property(_qtcore_loc Qt${QT_VERSION_MAJOR}::Core IMPORTED_LOCATION)
  endif()
  string(REGEX REPLACE "/Qt[A-Za-z0-9]+\\.framework/.*" "" _qt_lib_dir "${_qtcore_loc}")

  if(QWT_LIBRARY MATCHES "\\.framework/")
    string(REGEX REPLACE "/qwt\\.framework/.*" "" _qwt_lib_dir "${QWT_LIBRARY}")
  else()
    get_filename_component(_qwt_lib_dir "${QWT_LIBRARY}" DIRECTORY)
  endif()

  set(APPS "\${CMAKE_INSTALL_PREFIX}/xtalopt.app")
  install(CODE "
     include(BundleUtilities)
     set(_qt_plugins_absolute)
     foreach(_plugin ${QT_PLUGINS})
       if(_plugin)
         list(APPEND _qt_plugins_absolute \"\${CMAKE_INSTALL_PREFIX}/\${_plugin}\")
       endif()
     endforeach()
     fixup_bundle(\"${APPS}\" \"\${_qt_plugins_absolute}\" \"${_qt_lib_dir};${_qwt_lib_dir}\")
     " COMPONENT Runtime)

  if(XTALOPT_MACOS_SIGN_BUNDLE)
    find_program(XTALOPT_CODESIGN_EXECUTABLE codesign)
    if(XTALOPT_CODESIGN_EXECUTABLE)
      install(CODE "
        list(APPEND CMAKE_MODULE_PATH \"${CMAKE_SOURCE_DIR}/cmake\")
        include(SignMacBundle)
        if(EXISTS \"${APPS}\")
          xtalopt_sign_macos_bundle(\"${APPS}\"
                                    \"${XTALOPT_MACOS_CODESIGN_IDENTITY}\"
                                    \"${XTALOPT_CODESIGN_EXECUTABLE}\")
        endif()
        " COMPONENT Runtime)
    else()
      message(WARNING "codesign not found; the installed macOS bundle will remain unsigned.")
    endif()
  endif()
endif()

if(WIN32)
  set(_install_exe_suffix ".exe")
  set(LibLocation "bin")
else()
  set(_install_exe_suffix "")
  set(LibLocation "lib")
endif()

if(BUILD_INDEPENDENT_PACKAGE)
  # Install the Qt platform plugin for every Windows GUI build.
  if(MSVC)
    set(CMAKE_INSTALL_UCRT_LIBRARIES ON)
  endif()
  if(WIN32 AND BUILD_XTALOPT_GUI)
    include(MacroInstallQtPlugin)
    install_qt_plugin("Qt${QT_VERSION_MAJOR}::QWindowsIntegrationPlugin"
                      "bin" XTALOPT_WINDOWS_QT_PLUGINS)
    if(TARGET "Qt${QT_VERSION_MAJOR}::QWindowsVistaStylePlugin")
      include(MacroInstallStyle)
      InstallQWindowsVistaStyle("bin" XTALOPT_WINDOWS_QT_PLUGINS)
    endif()
  elseif(UNIX AND NOT APPLE AND BUILD_XTALOPT_GUI)
    include(MacroInstallPlatform)
    InstallQXcbPlatform("bin/platforms" "lib")
  endif()

  include(InstallRequiredSystemLibraries)

  if(APPLE AND NOT BUILD_XTALOPT_GUI)
    if(XTALOPT_MACOS_SIGN_BUNDLE)
      find_program(XTALOPT_CODESIGN_EXECUTABLE codesign)
    else()
      set(XTALOPT_CODESIGN_EXECUTABLE "XTALOPT_CODESIGN_EXECUTABLE-NOTFOUND")
    endif()
    install(CODE "
      list(APPEND CMAKE_MODULE_PATH \"${CMAKE_SOURCE_DIR}/cmake\")
      include(BundleUtilities)
      include(SignMacBundle)
      set(_exe \"\${CMAKE_INSTALL_PREFIX}/bin/xtalopt${_install_exe_suffix}\")
      set(_dep_dirs \"${DEP_SEARCH_DIRS}\")
      set(_frameworks_dir \"\${CMAKE_INSTALL_PREFIX}/bin/Frameworks\")
      function(gp_item_default_embedded_path_override item path_var)
        if(item MATCHES \"[^/]+\\\\.framework/\" OR item MATCHES \"\\\\.dylib$\")
          set(path \"@executable_path/Frameworks\" PARENT_SCOPE)
        endif()
      endfunction()
      set(BU_CHMOD_BUNDLE_ITEMS TRUE)
      unset(BU_COPY_FULL_FRAMEWORK_CONTENTS)
      fixup_bundle(\"\${_exe}\" \"\" \"\${_dep_dirs}\")
      if(NOT ${XTALOPT_MACOS_SIGN_BUNDLE})
        message(STATUS \"Skipping codesign for the installed macOS CLI tree.\")
      elseif(\"${XTALOPT_CODESIGN_EXECUTABLE}\" STREQUAL \"XTALOPT_CODESIGN_EXECUTABLE-NOTFOUND\")
        message(WARNING \"codesign not found; the installed macOS CLI tree may not run after install_name_tool rewrites.\")
      else()
        xtalopt_sign_macos_cli_install_tree(\"\${_exe}\"
                                            \"\${_frameworks_dir}\"
                                            \"${XTALOPT_MACOS_CODESIGN_IDENTITY}\"
                                            \"${XTALOPT_CODESIGN_EXECUTABLE}\")
      endif()
    ")
  elseif(NOT APPLE)
    install(CODE "
      list(APPEND CMAKE_MODULE_PATH \"${CMAKE_SOURCE_DIR}/cmake\")
      include(MacroInstallDependencies)
      set(_exe \"\${CMAKE_INSTALL_PREFIX}/bin/xtalopt${_install_exe_suffix}\")
      set(_dep_dirs \"${DEP_SEARCH_DIRS}\")
      InstallResolvedDependencies(\"\${_exe}\" \"\${CMAKE_INSTALL_PREFIX}/${LibLocation}\" \"\${_dep_dirs}\")
    ")
  endif()
endif()
