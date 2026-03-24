#include "server/net/session.h"

namespace server {

Session::Session(std::uint64_t session_id) : session_id_(session_id) {}

std::uint64_t Session::session_id() const { return session_id_; }

SessionState Session::state() const { return state_; }

const std::optional<shared::PlayerId>& Session::player_id() const {
  return player_id_;
}

void Session::Authenticate(shared::PlayerId player_id) {
  player_id_ = player_id;
  state_ = SessionState::kAuthenticated;
}

void Session::SelectCharacter() {
  if (state_ != SessionState::kAuthenticated) {
    return;
  }
  state_ = SessionState::kCharacterSelected;
}

void Session::EnterScene() {
  if (state_ != SessionState::kCharacterSelected &&
      state_ != SessionState::kAuthenticated) {
    return;
  }
  state_ = SessionState::kInScene;
}

void Session::Disconnect() { state_ = SessionState::kDisconnected; }

}  // namespace server
