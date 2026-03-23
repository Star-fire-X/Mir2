# Legend MMORPG Dual-End Parallel Worktree Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the unfinished tasks from the dual-end implementation plan into a low-conflict multi-worktree execution schedule that can start immediately from the current verified Task 1-2 baseline.

**Architecture:** `docs/superpowers/plans/2026-03-22-legend-mmorpg-dual-end-implementation-plan.md` remains the functional source of truth for code and test detail. This companion plan only defines worktree boundaries, file ownership, dependency gates, merge order, and verification checkpoints. Parallelism is deliberately limited by hot files such as `server/CMakeLists.txt`, `client/CMakeLists.txt`, `server/scene/scene.cpp`, and `server/protocol/protocol_dispatcher.cpp`, so those files are assigned to one active lane at a time.

**Tech Stack:** Git worktree, Git branches, CMake, GoogleTest, current C++20 server/client repository

---

## 0. Source Plan and Current Status

- Source plan: `docs/superpowers/plans/2026-03-22-legend-mmorpg-dual-end-implementation-plan.md`
- Verified complete at time of writing: Task 1, Task 2
- Still marked complete in the source plan but not present in the repo: Task 3
- Remaining execution scope for this plan: Task 3 through Task 16
- Existing worktree directory: `.worktrees/`
- Verified ignore rule: `.gitignore` already contains `.worktrees/`

## 1. Ownership Rules

- Only one active worktree may own `server/scene/scene.cpp` at a time.
- Only one active worktree may own `server/protocol/protocol_dispatcher.cpp` at a time.
- Only one active worktree may own `server/CMakeLists.txt` at a time, and only one active worktree may own `client/CMakeLists.txt` at a time.
- Do not edit `shared/protocol/*.h` from multiple lanes. If the contracts must change, cut a dedicated short-lived `dualend/contract-sync` branch, merge it first, then rebase the affected server and client lanes.
- Downstream worktrees branch from `dualend/integration-base` after the required gates are merged. Do not branch from a sibling feature branch that has not been merged yet.

### Task 1: Lock the Base Ref and Integration Branch

**Files:**
- Reference: `docs/superpowers/plans/2026-03-22-legend-mmorpg-dual-end-implementation-plan.md`
- Reference: `.gitignore`
- Verify: `CMakeLists.txt`
- Verify: `server/CMakeLists.txt`
- Verify: `client/CMakeLists.txt`

- [ ] **Step 1: Freeze the current code baseline into an integration branch**

Run:

```bash
BASE_REF="$(git rev-parse HEAD)"
git branch dualend/integration-base "${BASE_REF}"
```

Expected: `dualend/integration-base` points at the commit that already contains the verified Task 1-2 baseline.

- [ ] **Step 2: Verify the worktree directory is safe to use**

Run:

```bash
test -d .worktrees
git check-ignore -q .worktrees
```

Expected: `.worktrees/` exists and is ignored.

- [ ] **Step 3: Verify the baseline still builds before parallel work starts**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "ServerBootstrapTest|SharedContractTest"
```

Expected: `ServerBootstrapTest` and `SharedContractTest` pass on the verified baseline.

- [ ] **Step 4: Audit the repo against the source plan before launching parallel work**

Run:

```bash
ctest --test-dir build -N
find server/tests -maxdepth 3 -type f | sort
find server/config server/core -maxdepth 3 -type f 2>/dev/null | sort
```

Expected:
- completed source-plan work is actually present in the tree
- if `ConfigValidatorTest`, trace context files, or other Task 3 artifacts are missing, treat Task 3 as unfinished work and execute Task 2 below before launching the later server worktrees

- [ ] **Step 5: Resolve the file extension convention once**

Run:

```bash
rg --files server client shared | rg '\.(cc|cpp)$'
```

Expected:
- choose one extension convention before Wave 1 starts
- current tree already uses `.cc` in multiple existing files, while the source plan names many new files as `.cpp`
- update the source plan or the implementation conventions once, then keep every later worktree consistent

- [ ] **Step 6: Record the hot files that require merge discipline**

Hot files:
- `server/CMakeLists.txt`
- `client/CMakeLists.txt`
- `server/scene/scene.cpp`
- `server/protocol/protocol_dispatcher.cpp`

### Task 2: Launch Worktree T0 - Logging, Trace Context, Config Validation

**Files:**
- Create/Modify: `server/app/server_app.h`
- Create/Modify: `server/app/server_app.cpp`
- Create/Modify: `server/core/log/logger.h`
- Create/Modify: `server/core/log/logger.cpp`
- Create/Modify: `server/core/log/trace_context.h`
- Create/Modify: `server/config/config_manager.h`
- Create/Modify: `server/config/config_manager.cpp`
- Create/Modify: `server/config/config_validator.h`
- Create/Modify: `server/tests/config/config_validator_test.cc`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Task 3

- [ ] **Step 1: Treat this task as blocking baseline repair**

Reason:
- the source plan marks Task 3 complete
- the current repo does not contain the expected logging, trace-context, or config-validator artifacts
- later server worktrees should not branch from a baseline that lies about config and trace coverage

- [ ] **Step 2: Create the Task 3 repair worktree from the integration base**

Run:

```bash
git worktree add .worktrees/dualend-task3-baseline -b dualend/task3-baseline dualend/integration-base
```

- [ ] **Step 3: Implement only original Task 3 in this worktree**

Constraints:
- own `server/app/server_app.*`, `server/core/log/**`, `server/config/**`, `server/tests/config/**`
- do not edit `server/scene/**`, `server/player/**`, `client/**`

- [ ] **Step 4: Run the Task 3 targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "ConfigValidatorTest|ServerBootstrapTest"
```

Expected: PASS.

- [ ] **Step 5: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/task3-baseline
```

Expected: Gate T0 complete.

### Task 3: Launch Worktree A - Persistence Foundation

**Files:**
- Create/Modify: `server/data/character_data.h`
- Create/Modify: `server/data/inventory.h`
- Create/Modify: `server/data/skill_book.h`
- Create/Modify: `server/db/player_repository.h`
- Create/Modify: `server/db/player_repository.cpp`
- Create/Modify: `server/db/save_service.h`
- Create/Modify: `server/db/save_service.cpp`
- Create/Modify: `server/tests/data/inventory_test.cpp`
- Create/Modify: `server/tests/data/save_service_test.cpp`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Task 4

- [ ] **Step 1: Wait until Gate T0 is merged**

Reason:
- Worktree A still modifies `server/CMakeLists.txt`
- do not overlap it with the Task 3 repair lane on the same hot file

- [ ] **Step 2: Create the persistence worktree from the integration base**

Run:

```bash
git worktree add .worktrees/dualend-persistence -b dualend/persistence dualend/integration-base
```

- [ ] **Step 3: Implement only original Task 4 in this worktree**

Constraints:
- own `server/data/**`, `server/db/**`, `server/tests/data/**`
- do not edit `server/protocol/**`, `server/scene/**`, `client/**`

- [ ] **Step 4: Run the Task 4 targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "InventoryTest|SaveServiceTest"
```

Expected: PASS.

- [ ] **Step 5: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/persistence
```

Expected: Gate A complete.

### Task 4: Launch Worktree B - Scene Foundation

**Files:**
- Create/Modify: `server/scene/scene.h`
- Create/Modify: `server/scene/scene.cpp`
- Create/Modify: `server/scene/scene_manager.h`
- Create/Modify: `server/ecs/components.h`
- Create/Modify: `server/ecs/system_context.h`
- Create/Modify: `server/entity/entity_factory.h`
- Create/Modify: `server/entity/entity_factory.cpp`
- Create/Modify: `server/tests/scene/scene_spawn_test.cpp`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Task 6

- [ ] **Step 1: Wait until Gate T0 and Gate A are merged**

Reason:
- Worktree B is logically independent from Worktree A
- but both lanes modify `server/CMakeLists.txt`
- serialize them so the published plan does not violate the hot-file rule

- [ ] **Step 2: Create the scene foundation worktree from the integration base**

Run:

```bash
git worktree add .worktrees/dualend-scene-foundation -b dualend/scene-foundation dualend/integration-base
```

- [ ] **Step 3: Implement only original Task 6 in this worktree**

Constraints:
- own `server/scene/**`, `server/ecs/**`, `server/entity/**`, `server/tests/scene/scene_spawn_test.cpp`
- do not edit `server/protocol/**`, `server/player/**`, `server/db/**`, `client/**`

- [ ] **Step 4: Run the Task 6 targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "SceneSpawnTest"
```

Expected: PASS.

- [ ] **Step 5: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/scene-foundation
```

Expected: Gate B complete.

### Task 5: Launch Worktree C - Client Bootstrap

**Files:**
- Create/Modify: `client/app/game_app.h`
- Create/Modify: `client/app/game_app.cpp`
- Create/Modify: `client/net/network_manager.h`
- Create/Modify: `client/net/network_manager.cpp`
- Create/Modify: `client/protocol/protocol_dispatcher.h`
- Create/Modify: `client/protocol/protocol_dispatcher.cpp`
- Create/Modify: `client/tests/net/network_queue_test.cpp`
- Create/Modify: `client/tests/protocol/dispatcher_test.cpp`
- Modify: `client/CMakeLists.txt`
- Source tasks: original Task 12

- [ ] **Step 1: Wait until Gate T0 is merged**

Reason:
- Wave 1 starts from the repaired baseline, even though Worktree C does not touch server files

- [ ] **Step 2: Create the client bootstrap worktree from the integration base**

Run:

```bash
git worktree add .worktrees/dualend-client-bootstrap -b dualend/client-bootstrap dualend/integration-base
```

- [ ] **Step 3: Implement only original Task 12 in this worktree**

Constraints:
- own `client/app/**`, `client/net/**`, `client/protocol/**`, `client/tests/net/**`, `client/tests/protocol/**`
- do not edit `server/**`

- [ ] **Step 4: Run the Task 12 targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "NetworkQueueTest|ClientDispatcherTest"
```

Expected: PASS.

- [ ] **Step 5: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/client-bootstrap
```

Expected: Gate C complete.

### Task 6: Launch Worktree D - Server Entry, Snapshot, AOI, Movement

**Files:**
- Create/Modify: `server/net/network_server.h`
- Create/Modify: `server/net/session.h`
- Create/Modify: `server/net/session.cpp`
- Create/Modify: `server/protocol/protocol_dispatcher.h`
- Create/Modify: `server/protocol/protocol_dispatcher.cpp`
- Create/Modify: `server/player/player.h`
- Create/Modify: `server/player/player.cpp`
- Create/Modify: `server/player/player_manager.h`
- Create/Modify: `server/aoi/aoi_system.h`
- Create/Modify: `server/aoi/aoi_system.cpp`
- Create/Modify: `server/scene/movement_system.h`
- Create/Modify: `server/scene/movement_system.cpp`
- Modify: `server/scene/scene.cpp`
- Create/Modify: `server/tests/player/player_login_flow_test.cpp`
- Create/Modify: `server/tests/aoi/aoi_enter_leave_test.cpp`
- Create/Modify: `server/tests/scene/enter_scene_snapshot_test.cpp`
- Create/Modify: `server/tests/scene/movement_system_test.cpp`
- Create/Modify: `server/tests/scene/move_correction_test.cpp`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Tasks 5, 7, 8

- [ ] **Step 1: Wait until Gate T0, Gate A, and Gate B are merged**

Required inputs:
- `CharacterData` and `SaveService` interfaces from Worktree A
- `Scene`, ECS components, and `EntityFactory` from Worktree B

- [ ] **Step 2: Create the server entry/movement worktree from the updated integration base**

Run:

```bash
git worktree add .worktrees/dualend-server-entry-movement -b dualend/server-entry-movement dualend/integration-base
```

- [ ] **Step 3: Implement original Tasks 5, 7, and 8 in one lane**

Why grouped:
- `server/protocol/protocol_dispatcher.cpp` is shared by Task 5 and Task 7
- `server/scene/scene.cpp` is shared by Task 7 and Task 8
- splitting these tasks would create constant rebases for the same two hot files

- [ ] **Step 4: Keep this worktree as the exclusive owner of server hot files during execution**

Exclusive hot-file ownership:
- `server/protocol/protocol_dispatcher.cpp`
- `server/scene/scene.cpp`

- [ ] **Step 5: Run the combined targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "PlayerLoginFlowTest|AoiEnterLeaveTest|EnterSceneSnapshotTest|MovementSystemTest|MoveCorrectionTest"
```

Expected: PASS.

- [ ] **Step 6: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/server-entry-movement
```

Expected: Gate D complete.

### Task 7: Launch Worktree E - Client Models, Views, Controllers, UI

**Files:**
- Create/Modify: `client/model/model_root.h`
- Create/Modify: `client/model/player_model.h`
- Create/Modify: `client/model/inventory_model.h`
- Create/Modify: `client/scene/scene_manager.h`
- Create/Modify: `client/scene/scene.h`
- Create/Modify: `client/view/entity_view.h`
- Create/Modify: `client/view/player_view.h`
- Create/Modify: `client/view/monster_view.h`
- Create/Modify: `client/view/drop_view.h`
- Create/Modify: `client/controller/player_controller.h`
- Create/Modify: `client/controller/player_controller.cpp`
- Create/Modify: `client/controller/skill_controller.h`
- Create/Modify: `client/controller/skill_controller.cpp`
- Create/Modify: `client/ui/ui_manager.h`
- Create/Modify: `client/ui/inventory_panel.h`
- Create/Modify: `client/debug/dev_panel.h`
- Create/Modify: `client/tests/scene/entity_mapping_test.cpp`
- Create/Modify: `client/tests/model/inventory_model_test.cpp`
- Create/Modify: `client/tests/controller/player_controller_test.cpp`
- Modify: `client/CMakeLists.txt`
- Source tasks: original Tasks 13, 14

- [ ] **Step 1: Wait until Gate C is merged**

Required inputs:
- `GameApp`
- `NetworkManager`
- client protocol dispatcher shell

- [ ] **Step 2: Create the client world/UI worktree from the updated integration base**

Run:

```bash
git worktree add .worktrees/dualend-client-world-ui -b dualend/client-world-ui dualend/integration-base
```

- [ ] **Step 3: Implement original Tasks 13 and 14 in one lane**

Why grouped:
- both tasks keep touching the same client-side state flow from dispatcher -> model -> scene/view -> UI
- separating them only defers model/UI integration pain into rebases

- [ ] **Step 4: Run the combined targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "EntityMappingTest|InventoryModelTest|PlayerControllerTest"
```

Expected: PASS.

- [ ] **Step 5: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/client-world-ui
```

Expected: Gate E complete.

### Task 8: Launch Worktree F - Server Combat, AI, Drop, Pickup

**Files:**
- Create/Modify: `server/skill/skill_system.h`
- Create/Modify: `server/skill/skill_system.cpp`
- Create/Modify: `server/battle/battle_system.h`
- Create/Modify: `server/battle/battle_system.cpp`
- Create/Modify: `server/buff/buff_system.h`
- Create/Modify: `server/buff/buff_system.cpp`
- Create/Modify: `server/ai/ai_system.h`
- Create/Modify: `server/ai/ai_system.cpp`
- Create/Modify: `server/scene/drop_system.h`
- Create/Modify: `server/scene/drop_system.cpp`
- Modify: `server/scene/scene.cpp`
- Create/Modify: `server/tests/battle/skill_cast_test.cpp`
- Create/Modify: `server/tests/battle/damage_resolution_test.cpp`
- Create/Modify: `server/tests/buff/buff_expire_test.cpp`
- Create/Modify: `server/tests/ai/monster_ai_test.cpp`
- Create/Modify: `server/tests/drop/pickup_idempotency_test.cpp`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Tasks 9, 10

- [ ] **Step 1: Wait until Gate D is merged**

Required inputs:
- fixed scene tick order
- movement correction flow
- authoritative AOI flush path

- [ ] **Step 2: Create the combat/drop worktree from the updated integration base**

Run:

```bash
git worktree add .worktrees/dualend-server-combat-drop -b dualend/server-combat-drop dualend/integration-base
```

- [ ] **Step 3: Implement original Tasks 9 and 10 in one lane**

Why grouped:
- both tasks extend the same scene tick and death/drop pipeline
- `server/scene/scene.cpp` would otherwise churn across multiple worktrees again

- [ ] **Step 4: Keep this worktree as the exclusive owner of `server/scene/scene.cpp` until merge**

Allowed shared edits:
- `server/CMakeLists.txt`
- new `server/skill/**`, `server/battle/**`, `server/buff/**`, `server/ai/**`, `server/scene/drop_system.*`

- [ ] **Step 5: Run the combined targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "SkillCastTest|DamageResolutionTest|BuffExpireTest|MonsterAiTest|PickupIdempotencyTest"
```

Expected: PASS.

- [ ] **Step 6: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/server-combat-drop
```

Expected: Gate F complete.

### Task 9: Launch Worktree G - Save, Logout, Disconnect, Reconnect

**Files:**
- Modify: `server/player/player.cpp`
- Modify: `server/db/save_service.cpp`
- Modify: `server/protocol/protocol_dispatcher.cpp`
- Create/Modify: `server/tests/player/logout_save_test.cpp`
- Create/Modify: `server/tests/player/reconnect_snapshot_test.cpp`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Task 11

- [ ] **Step 1: Wait until Gate T0, Gate A, Gate D, and Gate F are merged**

Required inputs:
- dirty-flag capable player state
- finished combat/drop side effects
- stable enter scene snapshot path

- [ ] **Step 2: Create the save/reconnect worktree from the updated integration base**

Run:

```bash
git worktree add .worktrees/dualend-save-reconnect -b dualend/save-reconnect dualend/integration-base
```

- [ ] **Step 3: Implement only original Task 11 in this worktree**

Constraints:
- own `server/player/player.cpp`, `server/db/save_service.cpp`, `server/protocol/protocol_dispatcher.cpp`, and reconnect tests
- do not edit `client/**`

- [ ] **Step 4: Run the Task 11 targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "LogoutSaveTest|ReconnectSnapshotTest"
```

Expected: PASS.

- [ ] **Step 5: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/save-reconnect
```

Expected: Gate G complete.

### Task 10: Launch Worktree H - Closed Loop Integration

**Files:**
- Create/Modify: `server/tests/integration/login_enter_move_combat_pickup_test.cpp`
- Create/Modify: `tools/replay/closed_loop_scenario.md`
- Modify: `server/app/server_app.cpp`
- Modify: `client/app/game_app.cpp`
- Source tasks: original Task 15

- [ ] **Step 1: Wait until Gate E and Gate G are merged**

Required inputs:
- client model/view/UI mainline
- server save/reconnect and runtime mainline

- [ ] **Step 2: Create the integration worktree from the updated integration base**

Run:

```bash
git worktree add .worktrees/dualend-closed-loop -b dualend/closed-loop dualend/integration-base
```

- [ ] **Step 3: Implement only original Task 15 in this worktree**

Constraints:
- own `server/app/server_app.cpp`, `client/app/game_app.cpp`, integration tests, and replay docs
- do not introduce new GM or tracing surfaces here

- [ ] **Step 4: Run the Task 15 targeted verification**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "ClosedLoopIntegrationTest"
```

Expected: PASS.

- [ ] **Step 5: Run the manual verification pass**

Run server and client locally and verify:
- login works
- initial snapshot appears
- movement correction is visible when needed
- combat drives HP change
- pickup updates inventory

- [ ] **Step 6: Merge this worktree immediately after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/closed-loop
```

Expected: Gate H complete.

### Task 11: Launch Worktree I - GM, Protocol Tracing, Hardening

**Files:**
- Create/Modify: `server/gm/gm_command_handler.h`
- Create/Modify: `server/gm/gm_command_handler.cpp`
- Create/Modify: `server/core/log/logger.h`
- Create/Modify: `server/core/log/logger.cpp`
- Create/Modify: `server/core/log/trace_context.h`
- Modify: `server/protocol/protocol_dispatcher.cpp`
- Create/Modify: `tools/proto_dump/proto_dump_main.cpp`
- Create/Modify: `server/tests/gm/gm_command_test.cpp`
- Create/Modify: `server/tests/drop/pickup_race_guard_test.cpp`
- Modify: `server/CMakeLists.txt`
- Source tasks: original Task 16

- [ ] **Step 1: Wait until Gate G is merged**

Required inputs:
- save retry path
- pickup idempotency path
- stable protocol dispatcher path

- [ ] **Step 2: Create the hardening worktree from the updated integration base**

Run:

```bash
git worktree add .worktrees/dualend-hardening -b dualend/hardening dualend/integration-base
```

- [ ] **Step 3: Implement original Task 16 in this worktree**

Constraints:
- own `server/gm/**`, `tools/proto_dump/**`, `server/tests/gm/**`, `server/tests/drop/pickup_race_guard_test.cpp`
- own protocol tracing edits in `server/core/log/logger.*`, `server/core/log/trace_context.h`, and `server/protocol/protocol_dispatcher.cpp`
- avoid `server/app/**` and `client/app/**`; if a trace hook appears to require those files, stop and split it into a separate reviewed follow-up

- [ ] **Step 4: Run the hardening verification set**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "GmCommandTest|PickupRaceGuardTest|SaveServiceTest"
```

Expected: PASS.

- [ ] **Step 5: Rebase or merge this worktree onto Gate H before final merge**

Reason:
- Task 16 final verification references `ClosedLoopIntegrationTest`
- keep functional hardening work parallel, but do not merge it before Task 15 is in

- [ ] **Step 6: Run the final Task 16 verification set**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "GmCommandTest|PickupRaceGuardTest|SaveServiceTest|ClosedLoopIntegrationTest"
```

Expected: PASS.

- [ ] **Step 7: Merge this worktree after verification**

Run:

```bash
git switch dualend/integration-base
git merge --no-ff dualend/hardening
```

Expected: Gate I complete.

## 2. Wave Summary

### Wave 0: Repair the false baseline first

- Worktree T0: `dualend/task3-baseline` -> original Task 3

### Wave 1: Safe parallel launch after Gate T0

- Worktree A: `dualend/persistence` -> original Task 4
- Worktree C: `dualend/client-bootstrap` -> original Task 12

### Wave 2: Follow-on lanes after early merges

- Worktree B: `dualend/scene-foundation` -> original Task 6
- Worktree E: `dualend/client-world-ui` -> original Tasks 13, 14

### Wave 3: Mid-game server mainline

- Worktree D: `dualend/server-entry-movement` -> original Tasks 5, 7, 8

### Wave 4: Server runtime expansion

- Worktree F: `dualend/server-combat-drop` -> original Tasks 9, 10

### Wave 5: Lifecycle completion

- Worktree G: `dualend/save-reconnect` -> original Task 11

### Wave 6: Late parallel finish

- Worktree H: `dualend/closed-loop` -> original Task 15
- Worktree I: `dualend/hardening` -> original Task 16

## 3. Merge Order and No-Go Conditions

### Required merge order

1. Gate T0
2. Gate A and Gate C
3. Gate B and Gate E
4. Gate D
5. Gate F
6. Gate G
7. Gate H and Gate I

### No-go conditions

- Do not start Worktree A, Worktree B, or Worktree C before Gate T0 is merged.
- Do not start Worktree B before Gate A is merged.
- Do not start Worktree E before Gate C is merged.
- Do not start Worktree D before Gate T0, Gate A, and Gate B are all merged.
- Do not start Worktree F before Gate D is merged.
- Do not start Worktree G before Gate F is merged.
- Do not merge Worktree I before Worktree H is merged or rebased in.
- Do not let two active worktrees change `server/scene/scene.cpp` or `server/protocol/protocol_dispatcher.cpp` simultaneously.

## 4. Contract-Sync Escape Hatch

If any lane discovers the shared protocol needs new fields or new message IDs:

- cut `dualend/contract-sync` from `dualend/integration-base`
- change only `shared/protocol/*.h`, `shared/types/*.h`, and the directly affected shared contract tests
- run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "SharedContractTest"
```

- merge `dualend/contract-sync` first
- rebase the active server and client lanes before continuing

## 5. Final Full Verification

After Gate I:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
git diff --check
```

## 6. Definition of Done

- All original Tasks 3-16 are merged through `dualend/integration-base`
- No unresolved ownership conflicts remain on the four hot files
- `ClosedLoopIntegrationTest` passes
- `SaveServiceTest` and `PickupRaceGuardTest` pass in the final tree
- Client and server manual smoke flow matches the original source plan exit criteria
