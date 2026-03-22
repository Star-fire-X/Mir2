#ifndef SHARED_TYPES_PLAYER_ID_H_
#define SHARED_TYPES_PLAYER_ID_H_

#include <compare>
#include <cstdint>
#include <ostream>

namespace shared {

struct PlayerId {
  std::uint64_t value = 0;
  auto operator<=>(const PlayerId&) const = default;
};

inline std::ostream& operator<<(std::ostream& stream,
                                const PlayerId& player_id) {
  stream << player_id.value;
  return stream;
}

}  // namespace shared

#endif  // SHARED_TYPES_PLAYER_ID_H_
