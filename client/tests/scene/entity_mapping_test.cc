#include "client/scene/scene.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(EntityMappingTest, EnterAoiCreatesView) {
  Scene scene;

  shared::VisibleEntitySnapshot entity;
  entity.entity_id = shared::EntityId{101};
  entity.kind = shared::VisibleEntityKind::kMonster;
  entity.position = shared::ScenePosition{3.0F, 5.0F};

  scene.EnterAoi(entity);

  const EntityView* view = scene.FindView(entity.entity_id);
  ASSERT_NE(view, nullptr);
  EXPECT_EQ(view->kind(), EntityViewKind::kMonster);
  EXPECT_EQ(scene.ViewCount(), 1U);
}

TEST(EntityMappingTest, LeaveAoiDestroysView) {
  Scene scene;

  shared::VisibleEntitySnapshot entity;
  entity.entity_id = shared::EntityId{202};
  entity.kind = shared::VisibleEntityKind::kDrop;
  entity.position = shared::ScenePosition{6.0F, 8.0F};
  scene.EnterAoi(entity);

  scene.LeaveAoi(entity.entity_id);

  EXPECT_EQ(scene.FindView(entity.entity_id), nullptr);
  EXPECT_EQ(scene.ViewCount(), 0U);
}

}  // namespace
}  // namespace client
