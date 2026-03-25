add_library(server_data STATIC
  data/character_data.cc
  data/inventory.cc)

target_include_directories(server_data PUBLIC ${PROJECT_SOURCE_DIR})
set_project_warnings(server_data)

add_library(server_scene STATIC
  scene/scene.cc
  scene/movement_system.cc
  scene/drop_system.cc
  entity/entity_factory.cc
  aoi/aoi_system.cc)

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

add_library(server_runtime STATIC
  skill/skill_system.cc
  battle/battle_system.cc
  buff/buff_system.cc
  ai/ai_system.cc
  net/session.cc
  player/player.cc
  protocol/protocol_dispatcher.cc)

target_include_directories(server_runtime PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(server_runtime PUBLIC server_scene server_db)
set_project_warnings(server_runtime)

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
target_link_libraries(server_app PRIVATE server_config server_runtime)
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
    player_repository_test
    tests/data/player_repository_test.cc)

  target_include_directories(player_repository_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    player_repository_test
    PRIVATE server_db
            GTest::gtest_main)
  set_project_warnings(player_repository_test)

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
            server_runtime
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

  add_executable(
    player_login_flow_test
    tests/player/player_login_flow_test.cc)
  target_include_directories(player_login_flow_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    player_login_flow_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(player_login_flow_test)

  add_executable(
    logout_save_test
    tests/player/logout_save_test.cc)
  target_include_directories(logout_save_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    logout_save_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(logout_save_test)

  add_executable(
    reconnect_snapshot_test
    tests/player/reconnect_snapshot_test.cc)
  target_include_directories(
    reconnect_snapshot_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    reconnect_snapshot_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(reconnect_snapshot_test)

  add_executable(
    aoi_enter_leave_test
    tests/aoi/aoi_enter_leave_test.cc)
  target_include_directories(aoi_enter_leave_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    aoi_enter_leave_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(aoi_enter_leave_test)

  add_executable(
    enter_scene_snapshot_test
    tests/scene/enter_scene_snapshot_test.cc)
  target_include_directories(
    enter_scene_snapshot_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    enter_scene_snapshot_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(enter_scene_snapshot_test)

  add_executable(
    movement_system_test
    tests/scene/movement_system_test.cc)
  target_include_directories(movement_system_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    movement_system_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(movement_system_test)

  add_executable(
    move_correction_test
    tests/scene/move_correction_test.cc)
  target_include_directories(move_correction_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    move_correction_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(move_correction_test)

  add_executable(
    skill_cast_test
    tests/battle/skill_cast_test.cc)
  target_include_directories(skill_cast_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    skill_cast_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(skill_cast_test)

  add_executable(
    damage_resolution_test
    tests/battle/damage_resolution_test.cc)
  target_include_directories(
    damage_resolution_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    damage_resolution_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(damage_resolution_test)

  add_executable(
    buff_expire_test
    tests/buff/buff_expire_test.cc)
  target_include_directories(buff_expire_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    buff_expire_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(buff_expire_test)

  add_executable(
    monster_ai_test
    tests/ai/monster_ai_test.cc)
  target_include_directories(monster_ai_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    monster_ai_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(monster_ai_test)

  add_executable(
    pickup_idempotency_test
    tests/drop/pickup_idempotency_test.cc)
  target_include_directories(
    pickup_idempotency_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    pickup_idempotency_test
    PRIVATE server_runtime
            GTest::gtest_main)
  set_project_warnings(pickup_idempotency_test)

  add_executable(
    closed_loop_integration_test
    tests/integration/login_enter_move_combat_pickup_test.cc
    app/server_app.cc)
  target_include_directories(
    closed_loop_integration_test PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(
    closed_loop_integration_test
    PRIVATE client_runtime
            server_config
            server_runtime
            GTest::gtest_main)
  set_project_warnings(closed_loop_integration_test)

  include(GoogleTest)
  gtest_discover_tests(inventory_test)
  gtest_discover_tests(player_repository_test)
  gtest_discover_tests(save_service_test)
  gtest_discover_tests(config_validator_test)
  gtest_discover_tests(logger_test)
  gtest_discover_tests(server_bootstrap_test)
  gtest_discover_tests(shared_contract_test)
  gtest_discover_tests(scene_spawn_test)
  gtest_discover_tests(player_login_flow_test)
  gtest_discover_tests(logout_save_test)
  gtest_discover_tests(reconnect_snapshot_test)
  gtest_discover_tests(aoi_enter_leave_test)
  gtest_discover_tests(enter_scene_snapshot_test)
  gtest_discover_tests(movement_system_test)
  gtest_discover_tests(move_correction_test)
  gtest_discover_tests(skill_cast_test)
  gtest_discover_tests(damage_resolution_test)
  gtest_discover_tests(buff_expire_test)
  gtest_discover_tests(monster_ai_test)
  gtest_discover_tests(pickup_idempotency_test)
  gtest_discover_tests(closed_loop_integration_test)
endif()
