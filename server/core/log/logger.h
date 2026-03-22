#ifndef SERVER_CORE_LOG_LOGGER_H_
#define SERVER_CORE_LOG_LOGGER_H_

#include "server/core/log/trace_context.h"

namespace server {

class Logger {
 public:
  Logger() = default;
  explicit Logger(TraceContext trace_context) : trace_context_(trace_context) {}

  void SetTraceContext(TraceContext trace_context) {
    trace_context_ = trace_context;
  }

  const TraceContext& trace_context() const { return trace_context_; }

 private:
  TraceContext trace_context_{};
};

}  // namespace server

#endif  // SERVER_CORE_LOG_LOGGER_H_
