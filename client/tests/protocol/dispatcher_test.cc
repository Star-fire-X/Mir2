#include <string>
#include <vector>

#include "client/net/network_manager.h"
#include "client/protocol/protocol_dispatcher.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(ClientDispatcherTest, DispatchesQueuedMessagesInArrivalOrder) {
  NetworkManager network_manager;
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::EnterSceneSnapshotMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{protocol::SelfStateMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{protocol::AoiEnterMessage{}});
  network_manager.Enqueue(protocol::ClientMessage{protocol::AoiLeaveMessage{}});
  network_manager.Enqueue(
      protocol::ClientMessage{protocol::InventoryDeltaMessage{}});

  ProtocolDispatcher dispatcher;
  std::vector<std::string> dispatch_order;

  dispatcher.SetEnterSceneSnapshotHandler(
      [&dispatch_order](const protocol::EnterSceneSnapshotMessage&) {
        dispatch_order.push_back("enter_scene_snapshot");
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

  for (const protocol::ClientMessage& message : network_manager.Drain()) {
    dispatcher.Dispatch(message);
  }

  EXPECT_EQ(dispatch_order,
            (std::vector<std::string>{
                "enter_scene_snapshot",
                "self_state",
                "aoi_enter",
                "aoi_leave",
                "inventory_delta",
            }));
}

}  // namespace
}  // namespace client
