#include "gtest/gtest.h"
#include "server/app/server_app.h"
#include "server/config/config_manager.h"
#include "server/net/session.h"

namespace server {
namespace {

GameConfig MakeValidConfig() {
  GameConfig config;
  config.item_templates.push_back(ItemTemplate{.id = 1});
  config.skill_templates.push_back(SkillTemplate{.id = 1, .range = 3});
  config.monster_templates.push_back(MonsterTemplate{
      .id = 1,
      .drop_item_id = 1,
      .move_speed = 1,
      .skill_id = 1,
  });
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 1});
  return config;
}

TEST(ServerBootstrapTest, ConfigManagerStartsEmpty) {
  const ConfigManager config_manager;
  EXPECT_FALSE(config_manager.IsLoaded());
}

TEST(ServerBootstrapTest, ConfigManagerCanBeExplicitlyLoaded) {
  ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());

  EXPECT_TRUE(config_manager.IsLoaded());
}

TEST(ServerBootstrapTest, ServerAppInitFailsWhenLoadedConfigIsInvalid) {
  ConfigManager config_manager;
  GameConfig config;
  config.monster_spawns.push_back(MonsterSpawn{.monster_template_id = 42});
  config_manager.Load(config);

  ServerApp app{config_manager};
  EXPECT_FALSE(app.Init());
}

TEST(ServerBootstrapTest, ServerAppInitSucceedsWhenLoadedConfigIsValid) {
  ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());

  ServerApp app{config_manager};
  EXPECT_TRUE(app.Init());
}

TEST(ServerBootstrapTest, InvalidEnterSceneRequestDoesNotBootstrapScene) {
  ConfigManager config_manager;
  config_manager.Load(MakeValidConfig());

  ServerApp app{config_manager};
  ASSERT_TRUE(app.Init());

  Session unauthenticated_session(11);
  EXPECT_TRUE(app.EnterScene(&unauthenticated_session,
                             shared::EnterSceneRequest{
                                 shared::PlayerId{999},
                                 77,
                             })
                  .empty());

  Session valid_session(12);
  const shared::LoginResponse login =
      app.Login(&valid_session, shared::LoginRequest{"hero", "pw"});
  ASSERT_EQ(login.error_code, shared::ErrorCode::kOk);

  const std::vector<ServerApp::OutboundEvent> enter_events = app.EnterScene(
      &valid_session, shared::EnterSceneRequest{login.player_id, 1});
  ASSERT_EQ(enter_events.size(), 1U);
  ASSERT_TRUE(
      std::holds_alternative<shared::EnterSceneSnapshot>(enter_events.front()));

  const shared::EnterSceneSnapshot& snapshot =
      std::get<shared::EnterSceneSnapshot>(enter_events.front());
  const auto monster_it = std::find_if(
      snapshot.visible_entities.begin(), snapshot.visible_entities.end(),
      [](const shared::VisibleEntitySnapshot& entity) {
        return entity.kind == shared::VisibleEntityKind::kMonster;
      });
  ASSERT_NE(monster_it, snapshot.visible_entities.end());
  EXPECT_EQ(monster_it->entity_id, shared::EntityId{900000});
}

}  // namespace
}  // namespace server
