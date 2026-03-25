# Closed Loop Scenario

## Goal

Exercise the first playable login -> enter scene -> move -> combat -> pickup
loop without manual data patching.

## Server Preconditions

- `GameConfig` contains:
  - item template `5001`
  - skill template `1001`
  - monster template `2001` dropping item `5001`
- scene `1` is bootstrapped before the player enters it
- the runtime movement budget allows the scripted move to complete in one tick

## Script

1. Send `LoginRequest{"hero", "pw"}` and expect `LoginResponse{kOk, player_id}`.
2. Send `EnterSceneRequest{player_id, 1}`.
3. Flush `EnterSceneSnapshot` and apply it on the client main thread.
4. Send `MoveRequest{controlled_entity_id, {6.0, 4.0}, client_seq=1}`.
5. Run one fixed-order tick:
   - command ingestion
   - movement
   - AOI refresh
   - protocol flush
6. Flush `SelfState` with the authoritative player position.
7. Send `CastSkillRequest{controlled_entity_id, monster_entity_id, 1001}`.
8. Resolve the hit, kill the monster, and spawn one drop.
9. Flush `AoiLeave(monster_entity_id)` before `AoiEnter(drop_entity_id)`.
10. Send `PickupRequest{player_id, drop_entity_id, client_seq=3}`.
11. Flush `AoiLeave(drop_entity_id)` before `InventoryDelta`.

## Expected Client State

- `PlayerModel.scene_id == 1`
- `PlayerModel.position == {6.0, 4.0}`
- the monster view is removed after combat
- the drop view appears after combat and disappears after pickup
- inventory slot `0` contains item template `5001` with count `1`
- the dev panel's latest protocol summary is `inventory_delta`
