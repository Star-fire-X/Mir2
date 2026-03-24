#ifndef SERVER_NET_NETWORK_SERVER_H_
#define SERVER_NET_NETWORK_SERVER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "server/net/session.h"

namespace server {

class NetworkServer {
 public:
  Session& Accept() {
    const std::uint64_t session_id = next_session_id_++;
    auto [it, _] = sessions_.try_emplace(session_id,
                                         std::make_unique<Session>(session_id));
    return *(it->second);
  }

  Session* Find(std::uint64_t session_id) {
    const auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

  [[nodiscard]] std::size_t session_count() const { return sessions_.size(); }

 private:
  std::uint64_t next_session_id_ = 1;
  std::unordered_map<std::uint64_t, std::unique_ptr<Session>> sessions_;
};

}  // namespace server

#endif  // SERVER_NET_NETWORK_SERVER_H_
