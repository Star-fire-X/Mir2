#include "gtest/gtest.h"
#include "server/buff/buff_system.h"
#include "server/ecs/components.h"

namespace server {
namespace {

TEST(BuffExpireTest, BuffExpiresAfterDuration) {
  BuffSystem buff_system;
  ecs::BuffContainerComponent buff_container;

  buff_system.AddOrRefresh(&buff_container, 3001, 5.0F);
  EXPECT_TRUE(buff_system.HasBuff(buff_container, 3001));

  buff_system.Tick(&buff_container, 3.0F);
  EXPECT_TRUE(buff_system.HasBuff(buff_container, 3001));

  buff_system.Tick(&buff_container, 2.5F);
  EXPECT_FALSE(buff_system.HasBuff(buff_container, 3001));
}

}  // namespace
}  // namespace server
