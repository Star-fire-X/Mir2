#ifndef SERVER_CORE_LOG_LOGGER_H_
#define SERVER_CORE_LOG_LOGGER_H_

#include <string>
#include <utility>
#include <vector>

#include "server/core/log/trace_context.h"

namespace server {

struct LogRecord {
  TraceContext trace_context {};
  std::string message;
};

class Logger {
 public:
  Logger() = default;
  explicit Logger(TraceContext trace_context)
      : trace_context_(trace_context) {}

  void SetTraceContext(TraceContext trace_context) {
    trace_context_ = trace_context;
  }

  void Log(std::string message) {
    records_.push_back(LogRecord{trace_context_, std::move(message)});
  }

  const std::vector<LogRecord>& records() const { return records_; }

 private:
  TraceContext trace_context_{};
  std::vector<LogRecord> records_;
};

}  // namespace server

#endif  // SERVER_CORE_LOG_LOGGER_H_
