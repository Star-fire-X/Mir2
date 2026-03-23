#include "client/net/network_manager.h"

#include <utility>
#include <vector>

namespace client {

void NetworkManager::Enqueue(Message message) {
  std::scoped_lock lock{mutex_};
  pending_messages_.push_back(std::move(message));
}

std::vector<NetworkManager::Message> NetworkManager::Drain() {
  std::scoped_lock lock{mutex_};
  std::vector<Message> drained;
  drained.swap(pending_messages_);
  return drained;
}

}  // namespace client
