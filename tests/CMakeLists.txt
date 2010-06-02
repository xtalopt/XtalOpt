INCLUDE_DIRECTORIES(${XtalOpt_BINARY_DIR}/src/xtalopt)

if(BUILD_TESTS)

  add_executable(checkTemplates checkTemplates.cpp)
  target_link_libraries(checkTemplates xtalopt)
  add_test(TemplateCheck checkTemplates)

  add_executable(checkStateFileGenerator checkStateFileGenerator.cpp)
  target_link_libraries(checkStateFileGenerator xtalopt)
  add_test(StateFileGeneratorCheck checkStateFileGenerator)

endif(BUILD_TESTS)