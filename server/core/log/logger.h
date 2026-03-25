#ifndef SERVER_CORE_LOG_LOGGER_H_
#define SERVER_CORE_LOG_LOGGER_H_

#include <optional>
#include <string>
#include <vector>

#include "server/core/log/trace_context.h"

namespace server {

struct LogRecord {
  TraceContext trace_context{};
  std::string message;
};

struct TraceFilter {
  std::optional<shared::PlayerId> player_id;
  std::optional<shared::EntityId> entity_id;
  std::optional<shared::MessageId> message_id;
};

class Logger {
 public:
  Logger() = default;
  explicit Logger(TraceContext trace_context);

  void SetTraceContext(TraceContext trace_context);
  void SetFilter(TraceFilter filter);
  void Log(std::string message);
  const std::vector<LogRecord>& records() const;

 private:
  bool MatchesFilter(const TraceContext& trace_context) const;

  TraceContext trace_context_{};
  TraceFilter filter_{};
  std::vector<LogRecord> records_;
};

}  // namespace server

#endif  // SERVER_CORE_LOG_LOGGER_H_
