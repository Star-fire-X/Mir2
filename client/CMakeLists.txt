add_library(client_runtime STATIC
  app/game_app.cc
  controller/player_controller.cc
  controller/skill_controller.cc
  net/network_manager.cc
  platform/sdl_runtime.cc
  protocol/protocol_dispatcher.cc)

target_include_directories(client_runtime PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(client_runtime PUBLIC asio_headers kcp_vendor
                                            shared_protocol SDL2::SDL2)
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
    network_runtime_test
    tests/net/network_runtime_test.cc)

  target_include_directories(network_runtime_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(network_runtime_test PRIVATE client_runtime
                                                    server_transport
                                                    GTest::gtest_main)
  set_project_warnings(network_runtime_test)

  add_executable(
    dispatcher_test
    tests/protocol/dispatcher_test.cc)

  target_include_directories(dispatcher_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(dispatcher_test PRIVATE client_runtime GTest::gtest_main)
  set_project_warnings(dispatcher_test)

  add_executable(
    sdl_runtime_test
    tests/app/sdl_runtime_test.cc)

  target_include_directories(sdl_runtime_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(sdl_runtime_test PRIVATE client_runtime GTest::gtest_main)
  set_project_warnings(sdl_runtime_test)

  add_executable(
    game_app_sdl_test
    tests/app/game_app_sdl_test.cc)

  target_include_directories(game_app_sdl_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(game_app_sdl_test PRIVATE client_runtime
                                                 server_transport
                                                 GTest::gtest_main)
  set_project_warnings(game_app_sdl_test)

  add_executable(
    entity_mapping_test
    tests/scene/entity_mapping_test.cc)

  target_include_directories(entity_mapping_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(entity_mapping_test PRIVATE client_runtime GTest::gtest_main)
  set_project_warnings(entity_mapping_test)

  add_executable(
    inventory_model_test
    tests/model/inventory_model_test.cc)

  target_include_directories(inventory_model_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(inventory_model_test PRIVATE client_runtime GTest::gtest_main)
  set_project_warnings(inventory_model_test)

  add_executable(
    player_controller_test
    tests/controller/player_controller_test.cc)

  target_include_directories(player_controller_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    player_controller_test
    PRIVATE client_runtime
            GTest::gtest_main)
  set_project_warnings(player_controller_test)

  include(GoogleTest)
  gtest_discover_tests(network_queue_test)
  gtest_discover_tests(network_runtime_test)
  gtest_discover_tests(dispatcher_test)
  gtest_discover_tests(sdl_runtime_test)
  gtest_discover_tests(game_app_sdl_test)
  gtest_discover_tests(entity_mapping_test)
  gtest_discover_tests(inventory_model_test)
  gtest_discover_tests(player_controller_test)
endif()
