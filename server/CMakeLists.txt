add_library(server_config STATIC config/config_manager.cc)

target_include_directories(server_config PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(server_config)

add_library(server_app_lib STATIC app/server_app.cc)
target_include_directories(server_app_lib PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(server_app_lib PUBLIC server_config)
set_project_warnings(server_app_lib)

add_executable(server_app app/server_main.cc)
target_link_libraries(server_app PRIVATE server_app_lib)
set_project_warnings(server_app)

if(BUILD_TESTING)
  add_executable(
    server_bootstrap_test
    tests/bootstrap/server_bootstrap_test.cc)

  target_include_directories(server_bootstrap_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    server_bootstrap_test
    PRIVATE server_app_lib
            server_config
            GTest::gtest_main)
  set_project_warnings(server_bootstrap_test)

  add_executable(
    shared_contract_test
    tests/protocol/shared_contract_test.cc)
  target_include_directories(shared_contract_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(shared_contract_test PRIVATE GTest::gtest_main)
  set_project_warnings(shared_contract_test)

  add_executable(
    config_validator_test
    tests/config/config_validator_test.cc)
  target_include_directories(config_validator_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(config_validator_test PRIVATE GTest::gtest_main)
  set_project_warnings(config_validator_test)

  include(GoogleTest)
  gtest_discover_tests(server_bootstrap_test)
  gtest_discover_tests(shared_contract_test)
  gtest_discover_tests(config_validator_test)
endif()
