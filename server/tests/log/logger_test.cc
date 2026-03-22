#include "gtest/gtest.h"
#include "server/core/log/logger.h"

namespace server {
namespace {

TEST(LoggerTest, PropagatesTraceContextIntoLogRecord) {
  Logger logger;
  TraceContext trace_context;
  trace_context.traceId = 7;
  trace_context.playerId = shared::PlayerId{11};
  trace_context.entityId = shared::EntityId{13};
  trace_context.clientSeq = 17;

  logger.SetTraceContext(trace_context);
  logger.Log("player joined");

  ASSERT_EQ(logger.records().size(), 1u);
  EXPECT_EQ(logger.records()[0].trace_context, trace_context);
  EXPECT_EQ(logger.records()[0].message, "player joined");
}

}  // namespace
}  // namespace server
