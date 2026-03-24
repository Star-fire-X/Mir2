#include "client/controller/skill_controller.h"

#include <utility>

namespace client {

SkillController::SkillController(CastSkillRequestSink cast_skill_request_sink)
    : cast_skill_request_sink_(std::move(cast_skill_request_sink)) {}

void SkillController::SetCastSkillRequestSink(
    CastSkillRequestSink cast_skill_request_sink) {
  cast_skill_request_sink_ = std::move(cast_skill_request_sink);
}

void SkillController::HandleSkillButton(shared::EntityId caster_entity_id,
                                        shared::EntityId target_entity_id,
                                        std::uint32_t skill_id,
                                        std::uint32_t client_seq) const {
  if (!cast_skill_request_sink_) {
    return;
  }

  cast_skill_request_sink_(shared::CastSkillRequest{
      caster_entity_id,
      target_entity_id,
      skill_id,
      client_seq,
  });
}

}  // namespace client
