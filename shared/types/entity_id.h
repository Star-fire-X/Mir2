#ifndef SHARED_TYPES_ENTITY_ID_H_
#define SHARED_TYPES_ENTITY_ID_H_

#include <compare>
#include <cstdint>
#include <ostream>

namespace shared {

struct EntityId {
  std::uint64_t value = 0;
  auto operator<=>(const EntityId&) const = default;
};

inline std::ostream& operator<<(std::ostream& stream,
                                const EntityId& entity_id) {
  stream << entity_id.value;
  return stream;
}

}  // namespace shared

#endif  // SHARED_TYPES_ENTITY_ID_H_
