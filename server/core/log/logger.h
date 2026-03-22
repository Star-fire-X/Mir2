#ifndef SERVER_CORE_LOG_LOGGER_H_
#define SERVER_CORE_LOG_LOGGER_H_

#include <ostream>
#include <string_view>

#include "server/core/log/trace_context.h"

namespace server {

class Logger {
 public:
  explicit Logger(std::ostream& stream) : stream_(stream) {}

  void Info(const TraceContext& trace_context, std::string_view message) {
    stream_ << "[trace_id=" << trace_context.trace_id
            << " player_id=" << trace_context.player_id
            << " entity_id=" << trace_context.entity_id
            << " client_seq=" << trace_context.client_seq << "] " << message
            << '\n';
  }

 private:
  std::ostream& stream_;
};

}  // namespace server

#endif  // SERVER_CORE_LOG_LOGGER_H_
