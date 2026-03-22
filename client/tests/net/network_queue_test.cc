#include <thread>
#include <variant>
#include <vector>

#include "client/net/network_manager.h"
#include "client/protocol/client_message.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(NetworkQueueTest, EnqueuesMessagesFromNetworkThreadInOrder) {
  NetworkManager network_manager;

  std::thread network_thread([&network_manager]() {
    network_manager.Enqueue(
        protocol::ClientMessage{protocol::EnterSceneSnapshotMessage{}});
    network_manager.Enqueue(protocol::ClientMessage{protocol::SelfStateMessage{}});
  });
  network_thread.join();

  const std::vector<protocol::ClientMessage> pending = network_manager.Drain();
  ASSERT_EQ(pending.size(), 2U);
  EXPECT_TRUE(
      std::holds_alternative<protocol::EnterSceneSnapshotMessage>(pending[0]));
  EXPECT_TRUE(std::holds_alternative<protocol::SelfStateMessage>(pending[1]));
}

}  // namespace
}  // namespace client
