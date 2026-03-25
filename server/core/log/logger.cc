#include "server/core/log/logger.h"

#include <string>
#include <utility>
#include <vector>

namespace server {

Logger::Logger(TraceContext trace_context) : trace_context_(trace_context) {}

void Logger::SetTraceContext(TraceContext trace_context) {
  trace_context_ = trace_context;
}

void Logger::SetFilter(TraceFilter filter) { filter_ = std::move(filter); }

void Logger::Log(std::string message) {
  if (!MatchesFilter(trace_context_)) {
    return;
  }

  records_.push_back(LogRecord{trace_context_, std::move(message)});
}

const std::vector<LogRecord>& Logger::records() const { return records_; }

bool Logger::MatchesFilter(const TraceContext& trace_context) const {
  if (filter_.player_id.has_value() &&
      trace_context.playerId != *filter_.player_id) {
    return false;
  }
  if (filter_.entity_id.has_value() &&
      trace_context.entityId != *filter_.entity_id) {
    return false;
  }
  if (filter_.message_id.has_value() &&
      trace_context.messageId != *filter_.message_id) {
    return false;
  }
  return true;
}

}  // namespace server
