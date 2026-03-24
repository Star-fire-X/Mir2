#ifndef SERVER_NET_SESSION_H_
#define SERVER_NET_SESSION_H_

#include <cstdint>
#include <optional>

#include "shared/types/player_id.h"

namespace server {

enum class SessionState : std::uint8_t {
  kConnected = 0,
  kAuthenticated = 1,
  kCharacterSelected = 2,
  kInScene = 3,
  kDisconnected = 4,
};

class Session {
 public:
  explicit Session(std::uint64_t session_id);

  [[nodiscard]] std::uint64_t session_id() const;
  [[nodiscard]] SessionState state() const;
  [[nodiscard]] const std::optional<shared::PlayerId>& player_id() const;

  void Authenticate(shared::PlayerId player_id);
  void SelectCharacter();
  void EnterScene();
  void Disconnect();

 private:
  std::uint64_t session_id_ = 0;
  SessionState state_ = SessionState::kConnected;
  std::optional<shared::PlayerId> player_id_;
};

}  // namespace server

#endif  // SERVER_NET_SESSION_H_
