add_library(server_config STATIC
  config/config_manager.cc
  config/config_validator.cc)

target_include_directories(server_config PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(server_config)

add_executable(server_app app/server_main.cc app/server_app.cc)
target_link_libraries(server_app PRIVATE server_config)
set_project_warnings(server_app)

if(BUILD_TESTING)
  add_executable(
    config_validator_test
    tests/config/config_validator_test.cc)

  target_include_directories(config_validator_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    config_validator_test
    PRIVATE server_config
            GTest::gtest_main)
  set_project_warnings(config_validator_test)

  add_executable(
    server_bootstrap_test
    tests/bootstrap/server_bootstrap_test.cc
    app/server_app.cc)

  target_include_directories(server_bootstrap_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    server_bootstrap_test
    PRIVATE server_config
            GTest::gtest_main)
  set_project_warnings(server_bootstrap_test)

  add_executable(
    shared_contract_test
    tests/protocol/shared_contract_test.cc)
  target_include_directories(shared_contract_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(shared_contract_test PRIVATE GTest::gtest_main)
  set_project_warnings(shared_contract_test)

  include(GoogleTest)
  gtest_discover_tests(config_validator_test)
  gtest_discover_tests(server_bootstrap_test)
  gtest_discover_tests(shared_contract_test)
endif()
