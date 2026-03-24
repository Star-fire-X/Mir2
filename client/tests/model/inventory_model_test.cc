#include "client/model/inventory_model.h"

#include "gtest/gtest.h"

namespace client {
namespace {

TEST(InventoryModelTest, InventoryDeltaUpdatesExpectedSlot) {
  InventoryModel inventory_model(4);
  shared::InventoryDelta delta;
  delta.slots.push_back(shared::InventorySlotDelta{2, /*item_template_id=*/5001,
                                                   /*item_count=*/8});

  inventory_model.ApplyDelta(delta);

  const auto& slots = inventory_model.slots();
  ASSERT_EQ(slots.size(), 4U);
  EXPECT_EQ(slots[2].item_template_id, 5001U);
  EXPECT_EQ(slots[2].item_count, 8U);
  EXPECT_TRUE(slots[2].occupied);
}

}  // namespace
}  // namespace client
