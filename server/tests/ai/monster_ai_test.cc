#include "gtest/gtest.h"
#include "server/ai/ai_system.h"

namespace server {
namespace {

TEST(MonsterAiTest, MonsterTransitionsIdleChaseAttackAndReturn) {
  AiSystem ai_system;
  MonsterAiState state = MonsterAiState::kIdle;

  state = ai_system.NextState(state, MonsterAiInput{
                                         true,
                                         6.0F,
                                         false,
                                         true,
                                     });
  EXPECT_EQ(state, MonsterAiState::kDetect);

  state = ai_system.NextState(state, MonsterAiInput{
                                         true,
                                         6.0F,
                                         false,
                                         true,
                                     });
  EXPECT_EQ(state, MonsterAiState::kChase);

  state = ai_system.NextState(state, MonsterAiInput{
                                         true,
                                         1.0F,
                                         false,
                                         false,
                                     });
  EXPECT_EQ(state, MonsterAiState::kAttack);

  state = ai_system.NextState(state, MonsterAiInput{
                                         false,
                                         1.0F,
                                         false,
                                         false,
                                     });
  EXPECT_EQ(state, MonsterAiState::kDisengage);

  state = ai_system.NextState(state, MonsterAiInput{
                                         false,
                                         1.0F,
                                         false,
                                         true,
                                     });
  EXPECT_EQ(state, MonsterAiState::kReturn);
}

}  // namespace
}  // namespace server
