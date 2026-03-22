#ifndef SHARED_TYPES_ERROR_CODES_H_
#define SHARED_TYPES_ERROR_CODES_H_

#include <cstdint>

namespace shared {

enum class ErrorCode : std::uint16_t {
  kOk = 0,
  kInvalidCredentials = 1,
  kInvalidScene = 2,
  kInvalidMove = 3,
  kSkillOnCooldown = 4,
  kInvalidSkillTarget = 5,
  kOutOfRange = 6,
  kDropNotFound = 7,
  kInventoryFull = 8,
};

}  // namespace shared

#endif  // SHARED_TYPES_ERROR_CODES_H_
