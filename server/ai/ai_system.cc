#include "server/ai/ai_system.h"

namespace server {

MonsterAiState AiSystem::NextState(MonsterAiState current_state,
                                   const MonsterAiInput& input) const {
  switch (current_state) {
    case MonsterAiState::kIdle:
      return input.has_target ? MonsterAiState::kDetect : MonsterAiState::kIdle;
    case MonsterAiState::kDetect:
      return input.has_target ? MonsterAiState::kChase : MonsterAiState::kIdle;
    case MonsterAiState::kChase:
      if (!input.has_target) {
        return MonsterAiState::kDisengage;
      }
      if (input.target_in_attack_range || input.distance_to_target <= 1.5F) {
        return MonsterAiState::kAttack;
      }
      return MonsterAiState::kChase;
    case MonsterAiState::kAttack:
      if (!input.has_target) {
        return MonsterAiState::kDisengage;
      }
      if (input.target_in_attack_range || input.distance_to_target <= 1.5F) {
        return MonsterAiState::kAttack;
      }
      return MonsterAiState::kChase;
    case MonsterAiState::kDisengage:
      return input.at_spawn ? MonsterAiState::kReturn
                            : MonsterAiState::kDisengage;
    case MonsterAiState::kReturn:
      return input.at_spawn ? MonsterAiState::kIdle : MonsterAiState::kReturn;
  }

  return MonsterAiState::kIdle;
}

}  // namespace server
