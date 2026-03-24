#include "client/controller/player_controller.h"

#include <optional>

#include "client/controller/skill_controller.h"
#include "client/model/model_root.h"
#include "client/protocol/client_message.h"
#include "client/ui/inventory_panel.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(PlayerControllerTest, MoveInputCreatesMoveRequest) {
  PlayerController player_controller;
  std::optional<shared::MoveRequest> captured_request;

  player_controller.SetMoveRequestSink(
      [&captured_request](const shared::MoveRequest& request) {
        captured_request = request;
      });

  player_controller.HandleMoveInput(shared::EntityId{3001},
                                    shared::ScenePosition{18.0F, 19.0F},
                                    /*client_seq=*/11,
                                    /*client_timestamp_ms=*/9012);

  ASSERT_TRUE(captured_request.has_value());
  EXPECT_EQ(captured_request->entity_id, shared::EntityId{3001});
  EXPECT_EQ(captured_request->target_position.x, 18.0F);
  EXPECT_EQ(captured_request->target_position.y, 19.0F);
  EXPECT_EQ(captured_request->client_seq, 11U);
  EXPECT_EQ(captured_request->client_timestamp_ms, 9012U);
}

TEST(PlayerControllerTest, SkillButtonRoutesThroughSkillController) {
  SkillController skill_controller;
  std::optional<shared::CastSkillRequest> captured_request;

  skill_controller.SetCastSkillRequestSink(
      [&captured_request](const shared::CastSkillRequest& request) {
        captured_request = request;
      });

  skill_controller.HandleSkillButton(shared::EntityId{3001},
                                     shared::EntityId{4001},
                                     /*skill_id=*/77, /*client_seq=*/13);

  ASSERT_TRUE(captured_request.has_value());
  EXPECT_EQ(captured_request->caster_entity_id, shared::EntityId{3001});
  EXPECT_EQ(captured_request->target_entity_id, shared::EntityId{4001});
  EXPECT_EQ(captured_request->skill_id, 77U);
  EXPECT_EQ(captured_request->client_seq, 13U);
}

TEST(PlayerControllerTest, InventoryPanelRefreshesFromModelUpdatesOnly) {
  ModelRoot model_root;
  InventoryPanel inventory_panel;
  inventory_panel.BindInventoryModel(&model_root.inventory_model());

  EXPECT_EQ(inventory_panel.refresh_count(), 0U);

  shared::InventoryDelta delta;
  delta.slots.push_back(shared::InventorySlotDelta{
      /*slot_index=*/1, /*item_template_id=*/6001, /*item_count=*/3});
  model_root.inventory_model().ApplyDelta(delta);

  EXPECT_EQ(inventory_panel.refresh_count(), 1U);
  ASSERT_EQ(inventory_panel.rendered_slots().size(), 20U);
  EXPECT_EQ(inventory_panel.rendered_slots()[1].item_template_id, 6001U);
  EXPECT_EQ(inventory_panel.rendered_slots()[1].item_count, 3U);
}

}  // namespace
}  // namespace client
