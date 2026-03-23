#ifndef CLIENT_NET_NETWORK_MANAGER_H_
#define CLIENT_NET_NETWORK_MANAGER_H_

#include <mutex>  // NOLINT(build/c++11)
#include <vector>

#include "client/protocol/client_message.h"

namespace client {

class NetworkManager {
 public:
  using Message = protocol::ClientMessage;

  void Enqueue(Message message);
  [[nodiscard]] std::vector<Message> Drain();

 private:
  std::mutex mutex_;
  std::vector<Message> pending_messages_;
};

}  // namespace client

#endif  // CLIENT_NET_NETWORK_MANAGER_H_
