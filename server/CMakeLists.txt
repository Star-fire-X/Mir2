add_library(server_data STATIC
  data/character_data.cc
  data/inventory.cc)

target_include_directories(server_data PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(server_data)

add_library(server_scene STATIC
  scene/scene.cc
  entity/entity_factory.cc)

target_include_directories(server_scene PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(server_scene PUBLIC EnTT::EnTT server_data)
get_target_property(
  ENTT_INCLUDE_DIRS
  EnTT::EnTT
  INTERFACE_INCLUDE_DIRECTORIES)
if(ENTT_INCLUDE_DIRS)
  target_include_directories(server_scene SYSTEM PUBLIC ${ENTT_INCLUDE_DIRS})
endif()
set_project_warnings(server_scene)

add_library(server_db STATIC
  db/player_repository.cc
  db/save_service.cc)

target_include_directories(server_db PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(server_db PUBLIC server_data)
set_project_warnings(server_db)

add_library(server_config STATIC
  config/config_manager.cc
  config/config_validator.cc)

target_include_directories(server_config PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(server_config)

add_executable(server_app app/server_main.cc app/server_app.cc)
target_link_libraries(server_app PRIVATE server_config)
set_project_warnings(server_app)

if(BUILD_TESTING)
  add_test(NAME ServerAppSmokeCheck COMMAND $<TARGET_FILE:server_app>)

  add_executable(
    inventory_test
    tests/data/inventory_test.cc)

  target_include_directories(inventory_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(inventory_test PRIVATE server_data GTest::gtest_main)
  set_project_warnings(inventory_test)

  add_executable(
    save_service_test
    tests/data/save_service_test.cc)

  target_include_directories(save_service_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(save_service_test PRIVATE server_db GTest::gtest_main)
  set_project_warnings(save_service_test)

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
    logger_test
    tests/log/logger_test.cc)

  target_include_directories(logger_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(logger_test PRIVATE GTest::gtest_main)
  set_project_warnings(logger_test)

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

  add_executable(
    scene_spawn_test
    tests/scene/scene_spawn_test.cc)
  target_include_directories(scene_spawn_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    scene_spawn_test
    PRIVATE server_scene
            GTest::gtest_main)
  set_project_warnings(scene_spawn_test)

  include(GoogleTest)
  gtest_discover_tests(inventory_test)
  gtest_discover_tests(save_service_test)
  gtest_discover_tests(config_validator_test)
  gtest_discover_tests(logger_test)
  gtest_discover_tests(server_bootstrap_test)
  gtest_discover_tests(shared_contract_test)
  gtest_discover_tests(scene_spawn_test)
endif()
