#ifndef SERVER_PLAYER_PLAYER_H_
#define SERVER_PLAYER_PLAYER_H_

#include <cstdint>
#include <optional>

#include "server/data/character_data.h"
#include "server/net/session.h"
#include "server/scene/scene.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace server {

class Player {
 public:
  explicit Player(CharacterData data);

  [[nodiscard]] const CharacterData& data() const;
  [[nodiscard]] CharacterData& mutable_data();

  [[nodiscard]] Session* session() const;
  [[nodiscard]] bool dirty() const;
  [[nodiscard]] const std::optional<shared::EntityId>& controlled_entity_id()
      const;
  [[nodiscard]] std::uint32_t current_scene_id() const;

  void BindSession(Session* session);
  void SetControlledEntity(shared::EntityId entity_id, std::uint32_t scene_id);
  void SubmitMove(Scene* scene, const shared::MoveRequest& move_request);
  void MarkDirty();
  void ClearDirty();

 private:
  CharacterData data_{};
  Session* session_ = nullptr;
  bool dirty_ = false;
  std::optional<shared::EntityId> controlled_entity_id_;
  std::uint32_t current_scene_id_ = 0;
};

}  // namespace server

#endif  // SERVER_PLAYER_PLAYER_H_
