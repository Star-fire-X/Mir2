#include "gtest/gtest.h"
#include "server/net/session.h"
#include "server/player/player_manager.h"
#include "server/protocol/protocol_dispatcher.h"
#include "server/scene/scene_manager.h"

namespace server {
namespace {

TEST(PlayerLoginFlowTest, LoginEnterSceneAndMoveQueueFollowLifecycleState) {
  PlayerManager player_manager;
  SceneManager scene_manager;
  ProtocolDispatcher dispatcher(&player_manager, &scene_manager);
  Session session(1);

  const shared::LoginResponse login = dispatcher.HandleLogin(
      &session, shared::LoginRequest{"hero_account", "password"});
  EXPECT_EQ(login.error_code, shared::ErrorCode::kOk);
  EXPECT_EQ(session.state(), SessionState::kCharacterSelected);

  const Player* player = player_manager.Find(login.player_id);
  ASSERT_NE(player, nullptr);
  EXPECT_EQ(player->session(), &session);

  const std::optional<shared::EnterSceneSnapshot> enter_scene_snapshot =
      dispatcher.HandleEnterScene(
          &session, shared::EnterSceneRequest{login.player_id, 1});
  ASSERT_TRUE(enter_scene_snapshot.has_value());
  EXPECT_EQ(session.state(), SessionState::kInScene);
  EXPECT_EQ(enter_scene_snapshot->player_id, login.player_id);

  const shared::MoveRequest move_request{
      enter_scene_snapshot->controlled_entity_id,
      shared::ScenePosition{2.0F, 1.0F},
      7,
      1234,
  };
  EXPECT_TRUE(dispatcher.HandleMoveRequest(&session, move_request));

  const Scene* scene = scene_manager.Find(1);
  ASSERT_NE(scene, nullptr);
  EXPECT_TRUE(scene->HasPendingCommands());

  session.Disconnect();
  EXPECT_EQ(session.state(), SessionState::kDisconnected);
}

}  // namespace
}  // namespace server
