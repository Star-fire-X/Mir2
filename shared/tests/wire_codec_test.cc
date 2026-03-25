#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "shared/protocol/combat_messages.h"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/wire_codec.h"

namespace shared {
namespace {

TEST(WireCodecTest, EnvelopeRoundTripsSceneChannelBootstrap) {
  const SceneChannelBootstrap bootstrap{
      .player_id = PlayerId{10001},
      .scene_id = 1,
      .kcp_conv = 77,
      .udp_port = 5001,
      .session_token = "token-abc",
  };

  const std::vector<std::uint8_t> encoded =
      EncodeEnvelope(bootstrap.kMessageId, EncodeMessage(bootstrap));
  const std::optional<Envelope> decoded = DecodeEnvelope(encoded);

  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->message_id, MessageId::kSceneChannelBootstrap);
  const std::optional<SceneChannelBootstrap> decoded_bootstrap =
      DecodeMessage<SceneChannelBootstrap>(decoded->payload);
  ASSERT_TRUE(decoded_bootstrap.has_value());
  EXPECT_EQ(decoded_bootstrap->player_id, bootstrap.player_id);
  EXPECT_EQ(decoded_bootstrap->scene_id, bootstrap.scene_id);
  EXPECT_EQ(decoded_bootstrap->kcp_conv, bootstrap.kcp_conv);
  EXPECT_EQ(decoded_bootstrap->udp_port, bootstrap.udp_port);
  EXPECT_EQ(decoded_bootstrap->session_token, bootstrap.session_token);
}

TEST(WireCodecTest, EnvelopeRoundTripsCastAndPickupResults) {
  const CastSkillResult cast_result{
      .caster_entity_id = EntityId{7001},
      .target_entity_id = EntityId{8001},
      .skill_id = 1001,
      .error_code = ErrorCode::kOk,
      .hp_delta = -80,
      .client_seq = 3,
  };
  const PickupResult pickup_result{
      .player_id = PlayerId{10001},
      .drop_entity_id = EntityId{500001},
      .error_code = ErrorCode::kOk,
      .client_seq = 4,
  };

  const std::optional<CastSkillResult> decoded_cast =
      DecodeMessage<CastSkillResult>(
          DecodeEnvelope(
              EncodeEnvelope(cast_result.kMessageId, EncodeMessage(cast_result)))
              ->payload);
  const std::optional<PickupResult> decoded_pickup =
      DecodeMessage<PickupResult>(DecodeEnvelope(EncodeEnvelope(
                                     pickup_result.kMessageId,
                                     EncodeMessage(pickup_result)))
                                     ->payload);

  ASSERT_TRUE(decoded_cast.has_value());
  ASSERT_TRUE(decoded_pickup.has_value());
  EXPECT_EQ(decoded_cast->target_entity_id, cast_result.target_entity_id);
  EXPECT_EQ(decoded_cast->hp_delta, cast_result.hp_delta);
  EXPECT_EQ(decoded_pickup->drop_entity_id, pickup_result.drop_entity_id);
  EXPECT_EQ(decoded_pickup->client_seq, pickup_result.client_seq);
}

}  // namespace
}  // namespace shared
