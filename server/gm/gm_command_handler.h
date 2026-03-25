#ifndef SERVER_GM_GM_COMMAND_HANDLER_H_
#define SERVER_GM_GM_COMMAND_HANDLER_H_

#include <string>
#include <vector>

#include "server/db/save_service.h"
#include "server/player/player.h"
#include "server/scene/drop_system.h"
#include "server/scene/scene.h"
#include "shared/protocol/scene_messages.h"

namespace server {

class GmCommandHandler {
 public:
  bool Teleport(Scene* scene, Player* player,
                const shared::ScenePosition& position) const;
  bool SpawnMonster(Scene* scene, std::uint32_t monster_template_id,
                    shared::EntityId entity_id,
                    const shared::ScenePosition& position) const;
  bool KillMonster(Scene* scene, shared::EntityId monster_entity_id,
                   DropSystem* drop_system,
                   const std::vector<DropItemSpec>& drop_items) const;
  bool AddItem(Player* player, std::uint32_t item_template_id,
               std::uint32_t item_count, std::uint32_t max_stack_size) const;
  std::string QueryPlayerState(const Player& player) const;
  bool ForceSave(Player* player, const Scene* scene,
                 SaveService* save_service) const;
};

}  // namespace server

#endif  // SERVER_GM_GM_COMMAND_HANDLER_H_
