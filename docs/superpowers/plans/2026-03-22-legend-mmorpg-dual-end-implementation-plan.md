# Legend MMORPG Dual-End Architecture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a first playable closed loop for a Legend-style MMORPG with a C++ server using EnTT ECS + OOP and a C++ client using a simple OOP architecture.

**Architecture:** The server keeps all high-frequency scene runtime in ECS and all low-frequency business state in OOP services. The client stays single-threaded for scene/UI updates, consumes authoritative state from the server, and rebuilds state on reconnect with full snapshots.

**Tech Stack:** C++20, CMake, EnTT, Asio/Boost.Asio, spdlog, GoogleTest, JSON/TOML config, SQL database repository layer

---

## 0. Scope, Assumptions, Exit Criteria

**Implementation scope**

- Single process server
- Single logic thread for scene runtime
- One baseline map
- Player, monster, drop entities
- Login, enter scene, move, combat, drop, pickup, inventory
- Timed save, logout save, reconnect full rebuild
- Logs, protocol tracing, GM, config validation, developer panel

**Not in scope**

- Multi-process split
- Trade, guild, mail, auction
- Complex client prediction and rollback
- Cross-server features

**Phase exit criteria**

- Closed loop can be executed end to end without manual data patching
- Server and client logs can be correlated by `trace_id` and `player_id`
- Pickup path is idempotent and no stable dup is reproducible
- Reconnect re-enters scene with a fresh full snapshot

## 1. Proposed File Structure

### Shared contract layer

**Files**
- Create: `shared/protocol/message_ids.h`
- Create: `shared/protocol/auth_messages.h`
- Create: `shared/protocol/scene_messages.h`
- Create: `shared/protocol/combat_messages.h`
- Create: `shared/types/entity_id.h`
- Create: `shared/types/player_id.h`
- Create: `shared/types/error_codes.h`

### Server bootstrap

**Files**
- Create: `server/app/server_main.cpp`
- Create: `server/app/server_app.h`
- Create: `server/app/server_app.cpp`
- Create: `server/core/log/logger.h`
- Create: `server/core/log/logger.cpp`
- Create: `server/core/time/game_clock.h`
- Create: `server/core/id/id_generator.h`
- Create: `server/config/config_manager.h`
- Create: `server/config/config_manager.cpp`
- Create: `server/tests/bootstrap/server_bootstrap_test.cpp`

### Server runtime

**Files**
- Create: `server/net/session.h`
- Create: `server/net/network_server.h`
- Create: `server/protocol/protocol_dispatcher.h`
- Create: `server/player/player.h`
- Create: `server/player/player_manager.h`
- Create: `server/scene/scene.h`
- Create: `server/scene/scene_manager.h`
- Create: `server/ecs/components.h`
- Create: `server/ecs/system_context.h`
- Create: `server/entity/entity_factory.h`
- Create: `server/aoi/aoi_system.h`
- Create: `server/ai/ai_system.h`
- Create: `server/skill/skill_system.h`
- Create: `server/battle/battle_system.h`
- Create: `server/buff/buff_system.h`
- Create: `server/scene/drop_system.h`

### Data and persistence

**Files**
- Create: `server/data/character_data.h`
- Create: `server/data/inventory.h`
- Create: `server/data/skill_book.h`
- Create: `server/db/player_repository.h`
- Create: `server/db/save_service.h`
- Create: `server/db/save_service.cpp`
- Create: `server/tests/data/inventory_test.cpp`
- Create: `server/tests/data/save_service_test.cpp`

### Client bootstrap and runtime

**Files**
- Create: `client/app/game_app.h`
- Create: `client/app/game_app.cpp`
- Create: `client/net/network_manager.h`
- Create: `client/protocol/protocol_dispatcher.h`
- Create: `client/model/model_root.h`
- Create: `client/model/player_model.h`
- Create: `client/model/inventory_model.h`
- Create: `client/scene/scene_manager.h`
- Create: `client/scene/scene.h`
- Create: `client/view/entity_view.h`
- Create: `client/view/player_view.h`
- Create: `client/view/monster_view.h`
- Create: `client/view/drop_view.h`
- Create: `client/controller/player_controller.h`
- Create: `client/controller/skill_controller.h`
- Create: `client/ui/ui_manager.h`
- Create: `client/debug/dev_panel.h`
- Create: `client/tests/model/inventory_model_test.cpp`

## 2. Implementation Order

### Task 1: Repository Bootstrap and Build Skeleton

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/Warnings.cmake`
- Create: `cmake/Dependencies.cmake`
- Create: `server/app/server_main.cpp`
- Create: `client/app/client_main.cpp`
- Test: `server/tests/bootstrap/server_bootstrap_test.cpp`

- [x] **Step 1: Create the top-level CMake layout**

```cmake
cmake_minimum_required(VERSION 3.25)
project(legend_mmorpg LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
enable_testing()
add_subdirectory(server)
add_subdirectory(client)
```

- [x] **Step 2: Write a failing bootstrap test**

```cpp
TEST(ServerBootstrapTest, ConfigManagerStartsEmpty) {
    ConfigManager cfg;
    EXPECT_FALSE(cfg.IsLoaded());
}
```

- [x] **Step 3: Run the bootstrap test and confirm it fails**

Run: `ctest --test-dir build --output-on-failure -R ServerBootstrapTest`

Expected: compile or link failure because `ConfigManager` is not implemented yet

- [x] **Step 4: Add minimal bootstrap code**

```cpp
int main() {
    return 0;
}
```

- [x] **Step 5: Re-run tests**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure -R ServerBootstrapTest`

Expected: bootstrap test passes

- [x] **Step 6: Commit**

```bash
git add CMakeLists.txt cmake server client
git commit -m "chore: bootstrap dual-end project skeleton"
```

### Task 2: Shared Protocol and ID Contracts

**Files:**
- Create: `shared/types/entity_id.h`
- Create: `shared/types/player_id.h`
- Create: `shared/types/error_codes.h`
- Create: `shared/protocol/message_ids.h`
- Create: `shared/protocol/auth_messages.h`
- Create: `shared/protocol/scene_messages.h`
- Create: `shared/protocol/combat_messages.h`
- Test: `server/tests/protocol/shared_contract_test.cpp`

- [x] **Step 1: Write failing contract tests**

```cpp
TEST(SharedContractTest, EntityIdIsComparableAndPrintable);
TEST(SharedContractTest, MessageIdsAreUnique);
```

- [x] **Step 2: Run the new tests**

Run: `ctest --test-dir build --output-on-failure -R SharedContractTest`

Expected: FAIL because shared headers do not exist

- [x] **Step 3: Implement ID wrappers and message enums**

```cpp
struct EntityId {
    uint64_t value {};
    auto operator<=>(const EntityId&) const = default;
};
```

- [x] **Step 4: Define first message families**

Include:
- login request/response
- enter scene request/snapshot
- move request/correction
- cast skill request/result
- pickup request/result
- inventory delta

- [x] **Step 5: Re-run contract tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R SharedContractTest`

Expected: PASS

- [x] **Step 6: Commit**

```bash
git add shared server/tests/protocol
git commit -m "feat: add shared protocol and id contracts"
```

### Task 3: Logging, Trace Context, Config Validation

**Files:**
- Create: `server/core/log/logger.h`
- Create: `server/core/log/trace_context.h`
- Create: `server/config/config_manager.h`
- Create: `server/config/config_validator.h`
- Create: `server/tests/config/config_validator_test.cpp`

- [ ] **Step 1: Write failing config validation tests**

Cover:
- missing monster template
- invalid drop item reference
- negative move speed
- invalid skill range

- [ ] **Step 2: Run config tests**

Run: `ctest --test-dir build --output-on-failure -R ConfigValidatorTest`

Expected: FAIL because validator is not implemented

- [ ] **Step 3: Implement trace context propagation**

```cpp
struct TraceContext {
    uint64_t traceId {};
    PlayerId playerId {};
    EntityId entityId {};
    uint32_t clientSeq {};
};
```

- [ ] **Step 4: Implement config loading and validation**

Minimum validation:
- referential integrity
- value ranges
- duplicate ids
- required fields

- [ ] **Step 5: Add startup-fast-fail path**

`ServerApp::Init()` must return false when config validation fails

- [ ] **Step 6: Re-run tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "ConfigValidatorTest|ServerBootstrapTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/core server/config server/tests/config
git commit -m "feat: add logging trace context and config validation"
```

### Task 4: Persistence Layer and Character Data Model

**Files:**
- Create: `server/data/character_data.h`
- Create: `server/data/inventory.h`
- Create: `server/data/skill_book.h`
- Create: `server/db/player_repository.h`
- Create: `server/db/player_repository.cpp`
- Create: `server/db/save_service.h`
- Create: `server/db/save_service.cpp`
- Test: `server/tests/data/inventory_test.cpp`
- Test: `server/tests/data/save_service_test.cpp`

- [ ] **Step 1: Write failing inventory tests**

Cover:
- add stackable item
- reject overflow
- merge into existing stack
- remove item from slot

- [ ] **Step 2: Write failing save service tests**

Cover:
- snapshot queued from main thread
- success callback clears dirty flag
- failure callback keeps retry flag

- [ ] **Step 3: Run data tests**

Run: `ctest --test-dir build --output-on-failure -R "InventoryTest|SaveServiceTest"`

Expected: FAIL because data and repository code are absent

- [ ] **Step 4: Implement `CharacterData` as long-lived state**

Must include:
- identity
- base attributes
- inventory
- learned skills
- last scene/position snapshot
- version

- [ ] **Step 5: Implement `SaveService` with snapshot versioning**

```cpp
struct PlayerSaveSnapshot {
    CharacterData data;
    uint64_t version {};
};
```

- [ ] **Step 6: Re-run data tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "InventoryTest|SaveServiceTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/data server/db server/tests/data
git commit -m "feat: add persistence layer and character data model"
```

### Task 5: Network Server, Session, Player Lifecycle

**Files:**
- Create: `server/net/network_server.h`
- Create: `server/net/session.h`
- Create: `server/net/session.cpp`
- Create: `server/protocol/protocol_dispatcher.h`
- Create: `server/protocol/protocol_dispatcher.cpp`
- Create: `server/player/player.h`
- Create: `server/player/player_manager.h`
- Test: `server/tests/player/player_login_flow_test.cpp`

- [ ] **Step 1: Write failing login flow test**

Cover:
- session accepted
- login request mapped to player
- duplicate login rejected

- [ ] **Step 2: Run player lifecycle tests**

Run: `ctest --test-dir build --output-on-failure -R PlayerLoginFlowTest`

Expected: FAIL because session/player code is missing

- [ ] **Step 3: Implement session state machine**

States:
- connected
- authenticated
- character-selected
- in-scene
- disconnected

- [ ] **Step 4: Implement `Player` as session and data facade**

Responsibilities:
- bind session
- hold `CharacterData`
- submit scene commands
- own dirty flags

- [ ] **Step 5: Implement protocol dispatcher for first auth messages**

Route:
- login req
- enter scene req
- disconnect

- [ ] **Step 6: Re-run player lifecycle tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R PlayerLoginFlowTest`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/net server/player server/protocol server/tests/player
git commit -m "feat: add server network session and player lifecycle"
```

### Task 6: Scene Skeleton, Entity Factory, ECS Components

**Files:**
- Create: `server/scene/scene.h`
- Create: `server/scene/scene.cpp`
- Create: `server/scene/scene_manager.h`
- Create: `server/ecs/components.h`
- Create: `server/ecs/system_context.h`
- Create: `server/entity/entity_factory.h`
- Create: `server/entity/entity_factory.cpp`
- Test: `server/tests/scene/scene_spawn_test.cpp`

- [ ] **Step 1: Write failing scene spawn tests**

Cover:
- player spawn creates required components
- monster spawn creates AI and hate components
- scene stores `EntityId` to `entt::entity` mapping

- [ ] **Step 2: Run scene spawn tests**

Run: `ctest --test-dir build --output-on-failure -R SceneSpawnTest`

Expected: FAIL because scene and entity factory are not implemented

- [ ] **Step 3: Implement minimal ECS components**

Include:
- identity
- position
- move state
- attr
- combat state
- skill runtime
- buff container
- aoi state
- player ref
- monster ref

- [ ] **Step 4: Implement `Scene` command queue and entity lookup**

```cpp
void Scene::Enqueue(SceneCommand cmd);
std::optional<entt::entity> Scene::Find(EntityId id) const;
```

- [ ] **Step 5: Implement `EntityFactory` spawn helpers**

- [ ] **Step 6: Re-run scene spawn tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R SceneSpawnTest`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/scene server/ecs server/entity server/tests/scene
git commit -m "feat: add scene skeleton and ecs entity factory"
```

### Task 7: Enter Scene Snapshot and AOI Baseline

**Files:**
- Create: `server/aoi/aoi_system.h`
- Create: `server/aoi/aoi_system.cpp`
- Modify: `server/scene/scene.cpp`
- Modify: `server/protocol/protocol_dispatcher.cpp`
- Test: `server/tests/aoi/aoi_enter_leave_test.cpp`
- Test: `server/tests/scene/enter_scene_snapshot_test.cpp`

- [ ] **Step 1: Write failing AOI tests**

Cover:
- initial enter scene snapshot contains self + visible entities
- moving across boundary emits enter/leave messages

- [ ] **Step 2: Run AOI tests**

Run: `ctest --test-dir build --output-on-failure -R "AoiEnterLeaveTest|EnterSceneSnapshotTest"`

Expected: FAIL because AOI and snapshot flush are missing

- [ ] **Step 3: Implement initial scene snapshot builder**

Must include:
- self state
- scene id
- visible entity list
- initial inventory summary

- [ ] **Step 4: Implement AOI visible set bookkeeping**

Message types:
- `S2C_AoiEnter`
- `S2C_AoiLeave`
- `S2C_EntityStateUpdate`

- [ ] **Step 5: Re-run AOI tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "AoiEnterLeaveTest|EnterSceneSnapshotTest"`

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add server/aoi server/scene server/protocol server/tests/aoi
git commit -m "feat: add enter scene snapshot and aoi baseline"
```

### Task 8: Movement Command, Tick Loop, Position Correction

**Files:**
- Create: `server/scene/movement_system.h`
- Create: `server/scene/movement_system.cpp`
- Modify: `server/scene/scene.cpp`
- Test: `server/tests/scene/movement_system_test.cpp`
- Test: `server/tests/scene/move_correction_test.cpp`

- [ ] **Step 1: Write failing movement tests**

Cover:
- legal move advances position
- overspeed request rejected
- blocked tile request corrected

- [ ] **Step 2: Run movement tests**

Run: `ctest --test-dir build --output-on-failure -R "MovementSystemTest|MoveCorrectionTest"`

Expected: FAIL because movement system is missing

- [ ] **Step 3: Implement move command ingestion**

`C2S_MoveReq` must carry:
- target position
- client seq
- client timestamp

- [ ] **Step 4: Implement fixed order tick**

Temporary order for now:
- commands
- movement
- aoi
- flush

- [ ] **Step 5: Implement correction responses**

Server sends authoritative position when divergence exceeds threshold

- [ ] **Step 6: Re-run movement tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "MovementSystemTest|MoveCorrectionTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/scene server/tests/scene
git commit -m "feat: add movement tick and correction flow"
```

### Task 9: Combat, Skill Runtime, Buff Runtime

**Files:**
- Create: `server/skill/skill_system.h`
- Create: `server/skill/skill_system.cpp`
- Create: `server/battle/battle_system.h`
- Create: `server/battle/battle_system.cpp`
- Create: `server/buff/buff_system.h`
- Create: `server/buff/buff_system.cpp`
- Test: `server/tests/battle/skill_cast_test.cpp`
- Test: `server/tests/battle/damage_resolution_test.cpp`
- Test: `server/tests/buff/buff_expire_test.cpp`

- [ ] **Step 1: Write failing combat tests**

Cover:
- cast fails on cooldown
- cast fails on invalid range
- cast consumes MP on success
- damage reduces HP
- buff expires on time

- [ ] **Step 2: Run combat tests**

Run: `ctest --test-dir build --output-on-failure -R "SkillCastTest|DamageResolutionTest|BuffExpireTest"`

Expected: FAIL because combat systems are absent

- [ ] **Step 3: Implement `SkillSystem` pre-checks**

Checks:
- skill exists
- cooldown ready
- enough MP
- target valid
- distance valid

- [ ] **Step 4: Implement `BattleSystem` resolution**

Outputs:
- combat result event
- hp delta
- death notice
- dirty flags

- [ ] **Step 5: Implement `BuffSystem` runtime**

Only support:
- add
- refresh
- expire
- periodic tick

- [ ] **Step 6: Re-run combat tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "SkillCastTest|DamageResolutionTest|BuffExpireTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/skill server/battle server/buff server/tests/battle server/tests/buff
git commit -m "feat: add combat skill and buff runtime"
```

### Task 10: Monster AI, Death, Drop Spawn, Pickup Flow

**Files:**
- Create: `server/ai/ai_system.h`
- Create: `server/ai/ai_system.cpp`
- Create: `server/scene/drop_system.h`
- Create: `server/scene/drop_system.cpp`
- Test: `server/tests/ai/monster_ai_test.cpp`
- Test: `server/tests/drop/pickup_idempotency_test.cpp`

- [ ] **Step 1: Write failing AI and pickup tests**

Cover:
- monster transitions idle -> chase -> attack -> return
- death spawns drop
- duplicate pickup request does not duplicate inventory items

- [ ] **Step 2: Run AI and drop tests**

Run: `ctest --test-dir build --output-on-failure -R "MonsterAiTest|PickupIdempotencyTest"`

Expected: FAIL because AI and drop systems are missing

- [ ] **Step 3: Implement six-state monster AI**

States:
- idle
- detect
- chase
- attack
- disengage
- return

- [ ] **Step 4: Implement death-to-drop flow**

Ordering:
1. resolve death
2. build drop list
3. create drop entities
4. flush spawn messages

- [ ] **Step 5: Implement pickup with idempotency cache**

Key:
- `player_id`
- `drop_entity_id`
- `client_seq`

- [ ] **Step 6: Re-run AI and drop tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "MonsterAiTest|PickupIdempotencyTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add server/ai server/scene server/tests/ai server/tests/drop
git commit -m "feat: add monster ai and drop pickup flow"
```

### Task 11: Save Triggers, Logout, Disconnect, Reconnect Rebuild

**Files:**
- Modify: `server/player/player.cpp`
- Modify: `server/db/save_service.cpp`
- Modify: `server/protocol/protocol_dispatcher.cpp`
- Test: `server/tests/player/logout_save_test.cpp`
- Test: `server/tests/player/reconnect_snapshot_test.cpp`

- [ ] **Step 1: Write failing save/reconnect tests**

Cover:
- logout schedules save
- disconnect freezes operations
- reconnect returns fresh full scene snapshot

- [ ] **Step 2: Run the save/reconnect tests**

Run: `ctest --test-dir build --output-on-failure -R "LogoutSaveTest|ReconnectSnapshotTest"`

Expected: FAIL because lifecycle completion logic is missing

- [ ] **Step 3: Implement dirty-flag based save triggers**

Triggers:
- timer
- inventory change
- resource change
- logout
- disconnect

- [ ] **Step 4: Implement reconnect as full rebuild**

Requirements:
- clear old scene binding
- clear old visible set
- send new snapshot

- [ ] **Step 5: Re-run save/reconnect tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "LogoutSaveTest|ReconnectSnapshotTest"`

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add server/player server/db server/protocol server/tests/player
git commit -m "feat: add save triggers logout and reconnect rebuild"
```

### Task 12: Client Bootstrap, Network Queue, Protocol Dispatch

**Files:**
- Create: `client/app/game_app.h`
- Create: `client/app/game_app.cpp`
- Create: `client/net/network_manager.h`
- Create: `client/net/network_manager.cpp`
- Create: `client/protocol/protocol_dispatcher.h`
- Create: `client/protocol/protocol_dispatcher.cpp`
- Test: `client/tests/net/network_queue_test.cpp`
- Test: `client/tests/protocol/dispatcher_test.cpp`

- [ ] **Step 1: Write failing client bootstrap tests**

Cover:
- network thread enqueues messages
- main thread dispatcher consumes them in order

- [ ] **Step 2: Run client bootstrap tests**

Run: `ctest --test-dir build --output-on-failure -R "NetworkQueueTest|ClientDispatcherTest"`

Expected: FAIL because client bootstrap and dispatch are missing

- [ ] **Step 3: Implement `GameApp` main loop shell**

Order:
- pump network
- input
- models
- scene
- ui
- render

- [ ] **Step 4: Implement `NetworkManager` queue handoff**

Background thread must never write Scene or UI directly

- [ ] **Step 5: Implement initial dispatcher routes**

Route:
- enter scene snapshot
- self state
- aoi enter/leave
- inventory delta

- [ ] **Step 6: Re-run client bootstrap tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "NetworkQueueTest|ClientDispatcherTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add client/app client/net client/protocol client/tests
git commit -m "feat: add client bootstrap and protocol dispatch"
```

### Task 13: Client Models, Scene Mapping, Entity Views

**Files:**
- Create: `client/model/model_root.h`
- Create: `client/model/player_model.h`
- Create: `client/model/inventory_model.h`
- Create: `client/scene/scene_manager.h`
- Create: `client/scene/scene.h`
- Create: `client/view/entity_view.h`
- Create: `client/view/player_view.h`
- Create: `client/view/monster_view.h`
- Create: `client/view/drop_view.h`
- Test: `client/tests/scene/entity_mapping_test.cpp`
- Test: `client/tests/model/inventory_model_test.cpp`

- [ ] **Step 1: Write failing scene/model tests**

Cover:
- `EnterAOI` creates view
- `LeaveAOI` destroys view
- `InventoryDelta` updates the expected slot

- [ ] **Step 2: Run scene/model tests**

Run: `ctest --test-dir build --output-on-failure -R "EntityMappingTest|InventoryModelTest"`

Expected: FAIL because client scene and models are absent

- [ ] **Step 3: Implement `ModelRoot` and focused models**

Must include:
- player model
- inventory model
- skill bar model
- scene state model

- [ ] **Step 4: Implement `Scene` entity mapping**

```cpp
std::unordered_map<EntityId, std::unique_ptr<EntityView>> _views;
```

- [ ] **Step 5: Implement basic `PlayerView`, `MonsterView`, `DropView`**

Behavior:
- init from snapshot
- apply delta
- update interpolation

- [ ] **Step 6: Re-run scene/model tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "EntityMappingTest|InventoryModelTest"`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add client/model client/scene client/view client/tests
git commit -m "feat: add client models scene mapping and entity views"
```

### Task 14: Client Controllers, UI Binding, Developer Panel

**Files:**
- Create: `client/controller/player_controller.h`
- Create: `client/controller/player_controller.cpp`
- Create: `client/controller/skill_controller.h`
- Create: `client/controller/skill_controller.cpp`
- Create: `client/ui/ui_manager.h`
- Create: `client/ui/inventory_panel.h`
- Create: `client/debug/dev_panel.h`
- Test: `client/tests/controller/player_controller_test.cpp`

- [ ] **Step 1: Write failing controller tests**

Cover:
- move input creates move request
- skill button routes through skill controller
- inventory panel refreshes from model updates only

- [ ] **Step 2: Run controller/UI tests**

Run: `ctest --test-dir build --output-on-failure -R PlayerControllerTest`

Expected: FAIL because controller and UI bindings are not implemented

- [ ] **Step 3: Implement `PlayerController` as input-to-request only**

Do not allow:
- direct UI mutation
- direct model mutation for authoritative state

- [ ] **Step 4: Implement `SkillController` and `InventoryPanel` binding**

Panel refresh path must be:
- protocol
- model
- panel

- [ ] **Step 5: Implement `DevPanel`**

Minimum fields:
- scene id
- ping
- entity count
- target id
- player position
- recent protocol summaries

- [ ] **Step 6: Re-run controller/UI tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R PlayerControllerTest`

Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add client/controller client/ui client/debug client/tests/controller
git commit -m "feat: add client controllers ui binding and dev panel"
```

### Task 15: End-to-End Closed Loop Integration

**Files:**
- Create: `server/tests/integration/login_enter_move_combat_pickup_test.cpp`
- Create: `tools/replay/closed_loop_scenario.md`
- Modify: `server/app/server_app.cpp`
- Modify: `client/app/game_app.cpp`

- [ ] **Step 1: Write the end-to-end integration scenario**

Sequence:
1. login
2. enter scene
3. move near monster
4. cast skill
5. monster dies
6. drop appears
7. pickup succeeds
8. inventory delta appears

- [ ] **Step 2: Run the integration scenario and confirm failure**

Run: `ctest --test-dir build --output-on-failure -R ClosedLoopIntegrationTest`

Expected: FAIL until all mainline pieces are wired together

- [ ] **Step 3: Wire server and client mainline paths**

Ensure:
- scene tick order is fixed
- protocol flush order is fixed
- client main thread is sole consumer of world state

- [ ] **Step 4: Re-run the integration scenario**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R ClosedLoopIntegrationTest`

Expected: PASS

- [ ] **Step 5: Manual verification**

Run server and client locally and verify:
- login works
- first scene snapshot appears
- movement correction is visible if needed
- combat result drives hp change
- pickup updates inventory

- [ ] **Step 6: Commit**

```bash
git add server client tools
git commit -m "feat: wire closed loop integration path"
```

### Task 16: GM Commands, Protocol Tracing, Hardening

**Files:**
- Create: `server/gm/gm_command_handler.h`
- Create: `server/gm/gm_command_handler.cpp`
- Create: `tools/proto_dump/proto_dump_main.cpp`
- Create: `server/tests/gm/gm_command_test.cpp`
- Create: `server/tests/drop/pickup_race_guard_test.cpp`

- [ ] **Step 1: Write failing GM and hardening tests**

Cover:
- spawn monster command
- add item command
- pickup duplicate request suppression
- save retry after simulated DB failure

- [ ] **Step 2: Run hardening tests**

Run: `ctest --test-dir build --output-on-failure -R "GmCommandTest|PickupRaceGuardTest|SaveServiceTest"`

Expected: FAIL because GM and some hardening paths are missing

- [ ] **Step 3: Implement GM commands and protocol dump tool**

Minimum commands:
- teleport
- spawn monster
- kill monster
- add item
- query player state
- force save

- [ ] **Step 4: Add filtered protocol tracing**

Filters:
- by player id
- by entity id
- by message id

- [ ] **Step 5: Re-run hardening tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure -R "GmCommandTest|PickupRaceGuardTest|SaveServiceTest|ClosedLoopIntegrationTest"`

Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add server/gm tools/proto_dump server/tests/gm server/tests/drop
git commit -m "feat: add gm tools protocol tracing and hardening guards"
```

## 3. Suggested Milestones

### Milestone A: Week 1

- Task 1 to Task 4 complete
- Build passes
- Config and data tests pass

### Milestone B: Week 2

- Task 5 to Task 8 complete
- Can log in, enter scene, move, and see AOI updates

### Milestone C: Week 3

- Task 9 to Task 11 complete
- Combat, death, drop, save, reconnect work on server

### Milestone D: Week 4

- Task 12 to Task 16 complete
- Client closed loop works end to end with GM and debug tooling

## 4. Verification Commands

Use these commands as the minimum verification set after each milestone:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

For targeted checks:

```bash
ctest --test-dir build --output-on-failure -R SceneSpawnTest
ctest --test-dir build --output-on-failure -R ClosedLoopIntegrationTest
ctest --test-dir build --output-on-failure -R PickupIdempotencyTest
```

## 5. Non-Negotiable Guardrails

- Never move long-lived business state into ECS just to “unify” architecture
- Never let the client UI mutate authoritative inventory, HP, drop, or reward state
- Never let network or DB threads write Scene state directly
- Never change pickup ordering from `validate -> inventory commit -> drop delete -> broadcast -> save dirty`
- Never implement reconnect as partial patching before full snapshot rebuild is stable

## 6. Delivery Checklist

- [ ] Shared contracts compiled by both server and client
- [ ] Server startup fails fast on bad config
- [ ] Login and enter scene path stable
- [ ] AOI enter/leave/state update path stable
- [ ] Combat path stable
- [ ] Drop pickup path idempotent
- [ ] Save and reconnect path stable
- [ ] Client entity mapping stable
- [ ] UI refresh driven by model only
- [ ] Logs, protocol tracing, GM, dev panel available
