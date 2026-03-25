#include "server/core/log/logger.h"

#include "gtest/gtest.h"

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

TEST(LoggerTest, AppliesPlayerEntityAndMessageFilters) {
  Logger logger;
  logger.SetFilter(TraceFilter{
      .player_id = shared::PlayerId{11},
      .entity_id = shared::EntityId{13},
      .message_id = shared::MessageId::kMoveRequest,
  });

  TraceContext skipped_context;
  skipped_context.traceId = 8;
  skipped_context.playerId = shared::PlayerId{11};
  skipped_context.entityId = shared::EntityId{13};
  skipped_context.clientSeq = 18;
  skipped_context.messageId = shared::MessageId::kEnterSceneRequest;
  logger.SetTraceContext(skipped_context);
  logger.Log("skip enter");

  TraceContext matched_context;
  matched_context.traceId = 9;
  matched_context.playerId = shared::PlayerId{11};
  matched_context.entityId = shared::EntityId{13};
  matched_context.clientSeq = 19;
  matched_context.messageId = shared::MessageId::kMoveRequest;
  logger.SetTraceContext(matched_context);
  logger.Log("trace move");

  ASSERT_EQ(logger.records().size(), 1u);
  EXPECT_EQ(logger.records()[0].trace_context, matched_context);
  EXPECT_EQ(logger.records()[0].message, "trace move");
}

}  // namespace
}  // namespace server
