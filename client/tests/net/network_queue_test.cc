#include <thread>  // NOLINT(build/c++11)
#include <variant>
#include <vector>

#include "client/net/network_manager.h"
#include "client/protocol/client_message.h"
#include "gtest/gtest.h"

namespace client {
namespace {

TEST(NetworkQueueTest, EnqueuesMessagesFromNetworkThreadInOrder) {
  NetworkManager network_manager;
  network_manager.Start();

  std::thread network_thread([&network_manager]() {
    network_manager.Enqueue(
        protocol::ClientMessage{protocol::EnterSceneSnapshotMessage{}});
    network_manager.Enqueue(
        protocol::ClientMessage{protocol::SelfStateMessage{}});
  });
  network_thread.join();

  const std::vector<protocol::ClientMessage> pending =
      network_manager.DrainInbound();
  ASSERT_EQ(pending.size(), 2U);
  EXPECT_TRUE(
      std::holds_alternative<protocol::EnterSceneSnapshotMessage>(pending[0]));
  EXPECT_TRUE(std::holds_alternative<protocol::SelfStateMessage>(pending[1]));
  network_manager.Stop();
}

TEST(NetworkQueueTest, StartAndStopTransitionConnectionState) {
  NetworkManager network_manager;

  EXPECT_EQ(network_manager.connection_state(), ConnectionState::kStopped);
  network_manager.Start();
  EXPECT_EQ(network_manager.connection_state(), ConnectionState::kRunning);
  network_manager.Stop();
  EXPECT_EQ(network_manager.connection_state(), ConnectionState::kStopped);
}

}  // namespace
}  // namespace client
