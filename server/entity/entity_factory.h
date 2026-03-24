#ifndef SERVER_ENTITY_ENTITY_FACTORY_H_
#define SERVER_ENTITY_ENTITY_FACTORY_H_

#include <cstdint>

#include "entt/entity/fwd.hpp"
#include "server/data/character_data.h"
#include "shared/protocol/scene_messages.h"
#include "shared/types/entity_id.h"

namespace server {

class Scene;

class EntityFactory {
 public:
  explicit EntityFactory(Scene* scene);

  entt::entity SpawnPlayer(const CharacterData& character_data,
                           shared::EntityId entity_id);

  entt::entity SpawnMonster(std::uint32_t monster_template_id,
                            shared::EntityId entity_id,
                            const shared::ScenePosition& spawn_position);

 private:
  entt::entity SpawnBaseEntity(shared::EntityId entity_id,
                               const shared::ScenePosition& spawn_position);

  Scene* scene_;
};

}  // namespace server

#endif  // SERVER_ENTITY_ENTITY_FACTORY_H_
