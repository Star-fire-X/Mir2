#ifndef CLIENT_MODEL_MODEL_ROOT_H_
#define CLIENT_MODEL_MODEL_ROOT_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "client/model/inventory_model.h"
#include "client/model/player_model.h"

namespace client {

class SkillBarModel {
 public:
  void SetSkillForSlot(std::size_t slot_index, std::uint32_t skill_id) {
    if (slot_index >= equipped_skill_ids_.size()) {
      equipped_skill_ids_.resize(slot_index + 1, 0);
    }
    equipped_skill_ids_[slot_index] = skill_id;
  }

  [[nodiscard]] const std::vector<std::uint32_t>& equipped_skill_ids() const {
    return equipped_skill_ids_;
  }

 private:
  std::vector<std::uint32_t> equipped_skill_ids_;
};

class SceneStateModel {
 public:
  void SetSceneId(std::uint32_t scene_id) { scene_id_ = scene_id; }
  void SetVisibleEntityCount(std::size_t visible_entity_count) {
    visible_entity_count_ = visible_entity_count;
  }

  [[nodiscard]] std::uint32_t scene_id() const { return scene_id_; }
  [[nodiscard]] std::size_t visible_entity_count() const {
    return visible_entity_count_;
  }

 private:
  std::uint32_t scene_id_ = 0;
  std::size_t visible_entity_count_ = 0;
};

class ModelRoot {
 public:
  PlayerModel& player_model() { return player_model_; }
  InventoryModel& inventory_model() { return inventory_model_; }
  SkillBarModel& skill_bar_model() { return skill_bar_model_; }
  SceneStateModel& scene_state_model() { return scene_state_model_; }

  const PlayerModel& player_model() const { return player_model_; }
  const InventoryModel& inventory_model() const { return inventory_model_; }
  const SkillBarModel& skill_bar_model() const { return skill_bar_model_; }
  const SceneStateModel& scene_state_model() const {
    return scene_state_model_;
  }

 private:
  PlayerModel player_model_;
  InventoryModel inventory_model_;
  SkillBarModel skill_bar_model_;
  SceneStateModel scene_state_model_;
};

}  // namespace client

#endif  // CLIENT_MODEL_MODEL_ROOT_H_
