#ifndef SERVER_AI_AI_SYSTEM_H_
#define SERVER_AI_AI_SYSTEM_H_

#include <cstdint>

namespace server {

enum class MonsterAiState : std::uint8_t {
  kIdle = 0,
  kDetect = 1,
  kChase = 2,
  kAttack = 3,
  kDisengage = 4,
  kReturn = 5,
};

struct MonsterAiInput {
  bool has_target = false;
  float distance_to_target = 0.0F;
  bool target_in_attack_range = false;
  bool at_spawn = true;
};

class AiSystem {
 public:
  [[nodiscard]] MonsterAiState NextState(MonsterAiState current_state,
                                         const MonsterAiInput& input) const;
};

}  // namespace server

#endif  // SERVER_AI_AI_SYSTEM_H_
