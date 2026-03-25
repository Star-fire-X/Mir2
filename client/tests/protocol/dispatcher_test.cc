#include <chrono>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include "client/app/game_app.h"
#include "client/net/network_manager.h"
#include "client/protocol/protocol_dispatcher.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(ClientDispatcherTest, DispatchesQueuedMessagesInArrivalOrder) {
  NetworkManager network_manager;
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::EnterSceneSnapshotMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{
      protocol::LoginResponseMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{
      protocol::SceneChannelBootstrapMessage{}});
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::SelfStateMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{protocol::AoiEnterMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{protocol::AoiLeaveMessage{}});
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::InventoryDeltaMessage{}});
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::CastSkillResultMessage{}});
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::PickupResultMessage{}});

  ProtocolDispatcher dispatcher;
  std::vector<std::string> dispatch_order;

  dispatcher.SetEnterSceneSnapshotHandler(
      [&dispatch_order](const protocol::EnterSceneSnapshotMessage&) {
        dispatch_order.push_back("enter_scene_snapshot");
      });
  dispatcher.SetLoginResponseHandler(
      [&dispatch_order](const protocol::LoginResponseMessage&) {
        dispatch_order.push_back("login_response");
      });
  dispatcher.SetSceneChannelBootstrapHandler(
      [&dispatch_order](const protocol::SceneChannelBootstrapMessage&) {
        dispatch_order.push_back("scene_channel_bootstrap");
      });
  dispatcher.SetSelfStateHandler(
      [&dispatch_order](const protocol::SelfStateMessage&) {
        dispatch_order.push_back("self_state");
      });
  dispatcher.SetAoiEnterHandler(
      [&dispatch_order](const protocol::AoiEnterMessage&) {
        dispatch_order.push_back("aoi_enter");
      });
  dispatcher.SetAoiLeaveHandler(
      [&dispatch_order](const protocol::AoiLeaveMessage&) {
        dispatch_order.push_back("aoi_leave");
      });
  dispatcher.SetInventoryDeltaHandler(
      [&dispatch_order](const protocol::InventoryDeltaMessage&) {
        dispatch_order.push_back("inventory_delta");
      });
  dispatcher.SetCastSkillResultHandler(
      [&dispatch_order](const protocol::CastSkillResultMessage&) {
        dispatch_order.push_back("cast_skill_result");
      });
  dispatcher.SetPickupResultHandler(
      [&dispatch_order](const protocol::PickupResultMessage&) {
        dispatch_order.push_back("pickup_result");
      });

  for (const protocol::ClientMessage& message : network_manager.DrainInbound()) {
    dispatcher.Dispatch(message);
  }

  EXPECT_EQ(dispatch_order, (std::vector<std::string>{
                                "enter_scene_snapshot",
                                "login_response",
                                "scene_channel_bootstrap",
                                "self_state",
                                "aoi_enter",
                                "aoi_leave",
                                "inventory_delta",
                                "cast_skill_result",
                                "pickup_result",
                            }));
}

TEST(ClientDispatcherTest, RunFramePumpsQueuedNetworkMessagesInArrivalOrder) {
  GameApp app;
  std::vector<std::string> dispatch_order;

  app.protocol_dispatcher().SetEnterSceneSnapshotHandler(
      [&dispatch_order](const protocol::EnterSceneSnapshotMessage&) {
        dispatch_order.push_back("enter_scene_snapshot");
      });
  app.protocol_dispatcher().SetLoginResponseHandler(
      [&dispatch_order](const protocol::LoginResponseMessage&) {
        dispatch_order.push_back("login_response");
      });
  app.protocol_dispatcher().SetSceneChannelBootstrapHandler(
      [&dispatch_order](const protocol::SceneChannelBootstrapMessage&) {
        dispatch_order.push_back("scene_channel_bootstrap");
      });
  app.protocol_dispatcher().SetSelfStateHandler(
      [&dispatch_order](const protocol::SelfStateMessage&) {
        dispatch_order.push_back("self_state");
      });
  app.protocol_dispatcher().SetAoiEnterHandler(
      [&dispatch_order](const protocol::AoiEnterMessage&) {
        dispatch_order.push_back("aoi_enter");
      });
  app.protocol_dispatcher().SetInventoryDeltaHandler(
      [&dispatch_order](const protocol::InventoryDeltaMessage&) {
        dispatch_order.push_back("inventory_delta");
      });
  app.protocol_dispatcher().SetCastSkillResultHandler(
      [&dispatch_order](const protocol::CastSkillResultMessage&) {
        dispatch_order.push_back("cast_skill_result");
      });
  app.protocol_dispatcher().SetPickupResultHandler(
      [&dispatch_order](const protocol::PickupResultMessage&) {
        dispatch_order.push_back("pickup_result");
      });

  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::EnterSceneSnapshotMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::LoginResponseMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::SceneChannelBootstrapMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::SelfStateMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::AoiEnterMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::InventoryDeltaMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::CastSkillResultMessage{}});
  app.network_manager().Enqueue(
      protocol::ClientMessage{protocol::PickupResultMessage{}});

  app.RunFrame();

  EXPECT_EQ(dispatch_order, (std::vector<std::string>{
                                "enter_scene_snapshot",
                                "login_response",
                                "scene_channel_bootstrap",
                                "self_state",
                                "aoi_enter",
                                "inventory_delta",
                                "cast_skill_result",
                                "pickup_result",
                            }));
  EXPECT_TRUE(app.network_manager().DrainInbound().empty());
}

TEST(ClientDispatcherTest, SameSceneSnapshotReplacesExistingSceneViews) {
  GameApp app;

  shared::EnterSceneSnapshot first_snapshot;
  first_snapshot.player_id = shared::PlayerId{1};
  first_snapshot.controlled_entity_id = shared::EntityId{1001};
  first_snapshot.scene_id = 1;
  first_snapshot.self_position = shared::ScenePosition{0.0F, 0.0F};
  first_snapshot.visible_entities = {
      shared::VisibleEntitySnapshot{
          shared::EntityId{1001},
          shared::VisibleEntityKind::kPlayer,
          shared::ScenePosition{0.0F, 0.0F},
      },
      shared::VisibleEntitySnapshot{
          shared::EntityId{2001},
          shared::VisibleEntityKind::kMonster,
          shared::ScenePosition{4.0F, 0.0F},
      },
      shared::VisibleEntitySnapshot{
          shared::EntityId{3001},
          shared::VisibleEntityKind::kDrop,
          shared::ScenePosition{5.0F, 0.0F},
      },
  };

  app.network_manager().Enqueue(protocol::ClientMessage{
      protocol::EnterSceneSnapshotMessage{first_snapshot}});
  app.RunFrame();

  const Scene* scene = app.scene_manager().Find(1);
  ASSERT_NE(scene, nullptr);
  EXPECT_EQ(scene->ViewCount(), 3U);
  ASSERT_NE(scene->FindView(shared::EntityId{2001}), nullptr);
  ASSERT_NE(scene->FindView(shared::EntityId{3001}), nullptr);

  shared::EnterSceneSnapshot replacement_snapshot = first_snapshot;
  replacement_snapshot.visible_entities = {
      shared::VisibleEntitySnapshot{
          shared::EntityId{1001},
          shared::VisibleEntityKind::kPlayer,
          shared::ScenePosition{8.0F, 1.0F},
      },
  };
  replacement_snapshot.self_position = shared::ScenePosition{8.0F, 1.0F};

  app.network_manager().Enqueue(protocol::ClientMessage{
      protocol::EnterSceneSnapshotMessage{replacement_snapshot}});
  app.RunFrame();

  scene = app.scene_manager().Find(1);
  ASSERT_NE(scene, nullptr);
  EXPECT_EQ(scene->ViewCount(), 1U);
  EXPECT_EQ(scene->FindView(shared::EntityId{2001}), nullptr);
  EXPECT_EQ(scene->FindView(shared::EntityId{3001}), nullptr);
  EXPECT_EQ(app.model_root().scene_state_model().visible_entity_count(), 1U);
  EXPECT_FLOAT_EQ(app.model_root().player_model().position().x, 8.0F);
  EXPECT_FLOAT_EQ(app.model_root().player_model().position().y, 1.0F);
}

TEST(ClientDispatcherTest, RunKeepsLoopAliveUntilStop) {
  GameApp app;
  bool run_returned = false;

  std::thread run_thread([&app, &run_returned]() {
    app.Run();
    run_returned = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_FALSE(run_returned);
  app.Stop();
  run_thread.join();
  EXPECT_TRUE(run_returned);
}

}  // namespace
}  // namespace client
