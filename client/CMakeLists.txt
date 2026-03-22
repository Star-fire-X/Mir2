add_library(client_runtime STATIC
  app/game_app.cc
  net/network_manager.cc
  protocol/protocol_dispatcher.cc)

target_include_directories(client_runtime PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(client_runtime)

add_executable(client_app app/client_main.cc)
target_link_libraries(client_app PRIVATE client_runtime)
set_project_warnings(client_app)

if(BUILD_TESTING)
  add_executable(
    network_queue_test
    tests/net/network_queue_test.cc)

  target_include_directories(network_queue_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(network_queue_test PRIVATE client_runtime GTest::gtest_main)
  set_project_warnings(network_queue_test)

  add_executable(
    dispatcher_test
    tests/protocol/dispatcher_test.cc)

  target_include_directories(dispatcher_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(dispatcher_test PRIVATE client_runtime GTest::gtest_main)
  set_project_warnings(dispatcher_test)

  include(GoogleTest)
  gtest_discover_tests(network_queue_test)
  gtest_discover_tests(dispatcher_test)
endif()
