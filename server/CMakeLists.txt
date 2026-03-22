add_library(server_config STATIC config/config_manager.cc)

target_include_directories(server_config PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(server_config)

add_executable(server_app app/server_main.cc)
target_link_libraries(server_app PRIVATE server_config)
set_project_warnings(server_app)

if(BUILD_TESTING)
  add_executable(
    server_bootstrap_test
    tests/bootstrap/server_bootstrap_test.cc)

  target_include_directories(server_bootstrap_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    server_bootstrap_test
    PRIVATE server_config
            GTest::gtest_main)
  set_project_warnings(server_bootstrap_test)

  include(GoogleTest)
  gtest_discover_tests(server_bootstrap_test)
endif()
