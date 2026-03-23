#include <cstdint>

#include "gtest/gtest.h"
#include "server/data/character_data.h"

namespace server {
namespace {

TEST(InventoryTest, AddsStackableItem) {
  Inventory inventory{3};

  EXPECT_TRUE(inventory.AddStackableItem(1, 3, 5));
  ASSERT_TRUE(inventory.slots()[0].has_value());
  EXPECT_EQ(inventory.slots()[0]->item_template_id, 1u);
  EXPECT_EQ(inventory.slots()[0]->item_count, 3u);
}

TEST(InventoryTest, RejectsOverflow) {
  Inventory inventory{1};

  EXPECT_FALSE(inventory.AddStackableItem(1, 6, 5));
  EXPECT_FALSE(inventory.slots()[0].has_value());
}

TEST(InventoryTest, RejectsOverflowWithoutPartiallyMutatingExistingStack) {
  Inventory inventory{2};

  ASSERT_TRUE(inventory.AddStackableItem(1, 4, 5));
  ASSERT_TRUE(inventory.AddStackableItem(2, 5, 5));

  EXPECT_FALSE(inventory.AddStackableItem(1, 2, 5));

  ASSERT_TRUE(inventory.slots()[0].has_value());
  ASSERT_TRUE(inventory.slots()[1].has_value());
  EXPECT_EQ(inventory.slots()[0]->item_template_id, 1u);
  EXPECT_EQ(inventory.slots()[0]->item_count, 4u);
  EXPECT_EQ(inventory.slots()[1]->item_template_id, 2u);
  EXPECT_EQ(inventory.slots()[1]->item_count, 5u);
}

TEST(InventoryTest, MergesIntoExistingStack) {
  Inventory inventory{2};

  EXPECT_TRUE(inventory.AddStackableItem(1, 2, 5));
  EXPECT_TRUE(inventory.AddStackableItem(1, 2, 5));

  ASSERT_TRUE(inventory.slots()[0].has_value());
  EXPECT_EQ(inventory.slots()[0]->item_template_id, 1u);
  EXPECT_EQ(inventory.slots()[0]->item_count, 4u);
  EXPECT_FALSE(inventory.slots()[1].has_value());
}

TEST(InventoryTest, RemovesItemFromSlot) {
  Inventory inventory{2};
  inventory.AddStackableItem(1, 2, 5);

  EXPECT_TRUE(inventory.RemoveItemFromSlot(0));
  EXPECT_FALSE(inventory.slots()[0].has_value());
}

}  // namespace
}  // namespace server
