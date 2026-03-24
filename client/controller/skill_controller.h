#ifndef CLIENT_CONTROLLER_SKILL_CONTROLLER_H_
#define CLIENT_CONTROLLER_SKILL_CONTROLLER_H_

#include <cstdint>
#include <functional>

#include "shared/protocol/combat_messages.h"
#include "shared/types/entity_id.h"

namespace client {

class SkillController {
 public:
  using CastSkillRequestSink =
      std::function<void(const shared::CastSkillRequest&)>;

  explicit SkillController(CastSkillRequestSink cast_skill_request_sink = {});

  void SetCastSkillRequestSink(CastSkillRequestSink cast_skill_request_sink);

  void HandleSkillButton(shared::EntityId caster_entity_id,
                         shared::EntityId target_entity_id,
                         std::uint32_t skill_id,
                         std::uint32_t client_seq) const;

 private:
  CastSkillRequestSink cast_skill_request_sink_;
};

}  // namespace client

#endif  // CLIENT_CONTROLLER_SKILL_CONTROLLER_H_
