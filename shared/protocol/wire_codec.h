#ifndef SHARED_PROTOCOL_WIRE_CODEC_H_
#define SHARED_PROTOCOL_WIRE_CODEC_H_

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "shared/protocol/auth_messages.h"
#include "shared/protocol/combat_messages.h"
#include "shared/protocol/runtime_messages.h"
#include "shared/protocol/scene_messages.h"

namespace shared {

struct Envelope {
  MessageId message_id = MessageId::kLoginRequest;
  std::vector<std::uint8_t> payload;
};

namespace detail {

template <typename T, bool IsEnum = std::is_enum_v<T>>
struct RawIntegralType {
  using type = T;
};

template <typename T>
struct RawIntegralType<T, true> {
  using type = std::underlying_type_t<T>;
};

template <typename T>
using RawIntegralTypeT = typename RawIntegralType<T>::type;

class BufferWriter {
 public:
  template <typename T>
  void WriteIntegral(T value) {
    static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
    using Raw = RawIntegralTypeT<T>;
    using Unsigned = std::make_unsigned_t<Raw>;
    Unsigned raw_value = static_cast<Unsigned>(value);
    for (std::size_t index = 0; index < sizeof(Unsigned); ++index) {
      bytes_.push_back(
          static_cast<std::uint8_t>((raw_value >> (index * 8U)) & 0xFFU));
    }
  }

  void WriteFloat(float value) {
    WriteIntegral(std::bit_cast<std::uint32_t>(value));
  }

  void WriteString(const std::string& value) {
    WriteIntegral<std::uint32_t>(static_cast<std::uint32_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void WritePlayerId(PlayerId player_id) { WriteIntegral(player_id.value); }
  void WriteEntityId(EntityId entity_id) { WriteIntegral(entity_id.value); }

  void WriteScenePosition(const ScenePosition& position) {
    WriteFloat(position.x);
    WriteFloat(position.y);
  }

  void WriteVisibleEntitySnapshot(const VisibleEntitySnapshot& snapshot) {
    WriteEntityId(snapshot.entity_id);
    WriteIntegral(snapshot.kind);
    WriteScenePosition(snapshot.position);
  }

  void WriteInventorySlotDelta(const InventorySlotDelta& slot_delta) {
    WriteIntegral(slot_delta.slot_index);
    WriteIntegral(slot_delta.item_template_id);
    WriteIntegral(slot_delta.item_count);
  }

  std::vector<std::uint8_t> TakeBytes() { return std::move(bytes_); }

 private:
  std::vector<std::uint8_t> bytes_;
};

class BufferReader {
 public:
  explicit BufferReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

  template <typename T>
  std::optional<T> ReadIntegral() {
    static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
    using Raw = RawIntegralTypeT<T>;
    using Unsigned = std::make_unsigned_t<Raw>;
    if (cursor_ + sizeof(Unsigned) > bytes_.size()) {
      return std::nullopt;
    }

    Unsigned value = 0;
    for (std::size_t index = 0; index < sizeof(Unsigned); ++index) {
      value = static_cast<Unsigned>(
          value | (static_cast<Unsigned>(bytes_[cursor_ + index])
                   << (index * 8U)));
    }
    cursor_ += sizeof(Unsigned);
    return static_cast<T>(value);
  }

  std::optional<float> ReadFloat() {
    const std::optional<std::uint32_t> raw = ReadIntegral<std::uint32_t>();
    if (!raw.has_value()) {
      return std::nullopt;
    }
    return std::bit_cast<float>(*raw);
  }

  std::optional<std::string> ReadString() {
    const std::optional<std::uint32_t> size = ReadIntegral<std::uint32_t>();
    if (!size.has_value() || cursor_ + *size > bytes_.size()) {
      return std::nullopt;
    }

    std::string value(bytes_.begin() + static_cast<std::ptrdiff_t>(cursor_),
                      bytes_.begin() +
                          static_cast<std::ptrdiff_t>(cursor_ + *size));
    cursor_ += *size;
    return value;
  }

  std::optional<PlayerId> ReadPlayerId() {
    const std::optional<std::uint64_t> value = ReadIntegral<std::uint64_t>();
    if (!value.has_value()) {
      return std::nullopt;
    }
    return PlayerId{*value};
  }

  std::optional<EntityId> ReadEntityId() {
    const std::optional<std::uint64_t> value = ReadIntegral<std::uint64_t>();
    if (!value.has_value()) {
      return std::nullopt;
    }
    return EntityId{*value};
  }

  std::optional<ScenePosition> ReadScenePosition() {
    const std::optional<float> x = ReadFloat();
    const std::optional<float> y = ReadFloat();
    if (!x.has_value() || !y.has_value()) {
      return std::nullopt;
    }
    return ScenePosition{*x, *y};
  }

  std::optional<VisibleEntitySnapshot> ReadVisibleEntitySnapshot() {
    const std::optional<EntityId> entity_id = ReadEntityId();
    const std::optional<VisibleEntityKind> kind =
        ReadIntegral<VisibleEntityKind>();
    const std::optional<ScenePosition> position = ReadScenePosition();
    if (!entity_id.has_value() || !kind.has_value() || !position.has_value()) {
      return std::nullopt;
    }
    return VisibleEntitySnapshot{*entity_id, *kind, *position};
  }

  std::optional<InventorySlotDelta> ReadInventorySlotDelta() {
    const std::optional<std::uint32_t> slot_index =
        ReadIntegral<std::uint32_t>();
    const std::optional<std::uint32_t> item_template_id =
        ReadIntegral<std::uint32_t>();
    const std::optional<std::uint32_t> item_count =
        ReadIntegral<std::uint32_t>();
    if (!slot_index.has_value() || !item_template_id.has_value() ||
        !item_count.has_value()) {
      return std::nullopt;
    }
    return InventorySlotDelta{*slot_index, *item_template_id, *item_count};
  }

  [[nodiscard]] bool consumed_all() const { return cursor_ == bytes_.size(); }

 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t cursor_ = 0;
};

}  // namespace detail

inline std::vector<std::uint8_t> EncodeEnvelope(
    MessageId message_id, std::span<const std::uint8_t> payload) {
  detail::BufferWriter writer;
  writer.WriteIntegral(message_id);
  writer.WriteIntegral<std::uint32_t>(static_cast<std::uint32_t>(payload.size()));
  std::vector<std::uint8_t> bytes = writer.TakeBytes();
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  return bytes;
}

inline std::optional<Envelope> DecodeEnvelope(std::span<const std::uint8_t> bytes) {
  detail::BufferReader reader(bytes);
  const std::optional<MessageId> message_id = reader.ReadIntegral<MessageId>();
  const std::optional<std::uint32_t> payload_size =
      reader.ReadIntegral<std::uint32_t>();
  if (!message_id.has_value() || !payload_size.has_value()) {
    return std::nullopt;
  }

  constexpr std::size_t kHeaderSize =
      sizeof(std::uint16_t) + sizeof(std::uint32_t);
  if (bytes.size() != kHeaderSize + *payload_size) {
    return std::nullopt;
  }

  Envelope envelope;
  envelope.message_id = *message_id;
  envelope.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize),
                          bytes.end());
  return envelope;
}

template <typename T>
std::vector<std::uint8_t> EncodeMessage(const T& message);

template <typename T>
std::optional<T> DecodeMessage(std::span<const std::uint8_t> payload);

template <>
inline std::vector<std::uint8_t> EncodeMessage(const LoginRequest& message) {
  detail::BufferWriter writer;
  writer.WriteString(message.account_name);
  writer.WriteString(message.password);
  return writer.TakeBytes();
}

template <>
inline std::optional<LoginRequest> DecodeMessage(std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<std::string> account_name = reader.ReadString();
  const std::optional<std::string> password = reader.ReadString();
  if (!account_name.has_value() || !password.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return LoginRequest{*account_name, *password};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const LoginResponse& message) {
  detail::BufferWriter writer;
  writer.WriteIntegral(message.error_code);
  writer.WritePlayerId(message.player_id);
  return writer.TakeBytes();
}

template <>
inline std::optional<LoginResponse> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<ErrorCode> error_code = reader.ReadIntegral<ErrorCode>();
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  if (!error_code.has_value() || !player_id.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return LoginResponse{*error_code, *player_id};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(
    const SceneChannelBootstrap& message) {
  detail::BufferWriter writer;
  writer.WritePlayerId(message.player_id);
  writer.WriteIntegral(message.scene_id);
  writer.WriteIntegral(message.kcp_conv);
  writer.WriteIntegral(message.udp_port);
  writer.WriteString(message.session_token);
  return writer.TakeBytes();
}

template <>
inline std::optional<SceneChannelBootstrap> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  const std::optional<std::uint32_t> scene_id =
      reader.ReadIntegral<std::uint32_t>();
  const std::optional<std::uint32_t> kcp_conv =
      reader.ReadIntegral<std::uint32_t>();
  const std::optional<std::uint16_t> udp_port =
      reader.ReadIntegral<std::uint16_t>();
  const std::optional<std::string> session_token = reader.ReadString();
  if (!player_id.has_value() || !scene_id.has_value() || !kcp_conv.has_value() ||
      !udp_port.has_value() || !session_token.has_value() ||
      !reader.consumed_all()) {
    return std::nullopt;
  }
  return SceneChannelBootstrap{
      *player_id, *scene_id, *kcp_conv, *udp_port, *session_token};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const EnterSceneRequest& message) {
  detail::BufferWriter writer;
  writer.WritePlayerId(message.player_id);
  writer.WriteIntegral(message.scene_id);
  return writer.TakeBytes();
}

template <>
inline std::optional<EnterSceneRequest> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  const std::optional<std::uint32_t> scene_id =
      reader.ReadIntegral<std::uint32_t>();
  if (!player_id.has_value() || !scene_id.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return EnterSceneRequest{*player_id, *scene_id};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(
    const EnterSceneSnapshot& message) {
  detail::BufferWriter writer;
  writer.WritePlayerId(message.player_id);
  writer.WriteEntityId(message.controlled_entity_id);
  writer.WriteIntegral(message.scene_id);
  writer.WriteScenePosition(message.self_position);
  writer.WriteIntegral<std::uint32_t>(
      static_cast<std::uint32_t>(message.visible_entities.size()));
  for (const VisibleEntitySnapshot& entity : message.visible_entities) {
    writer.WriteVisibleEntitySnapshot(entity);
  }
  return writer.TakeBytes();
}

template <>
inline std::optional<EnterSceneSnapshot> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  const std::optional<EntityId> controlled_entity_id = reader.ReadEntityId();
  const std::optional<std::uint32_t> scene_id =
      reader.ReadIntegral<std::uint32_t>();
  const std::optional<ScenePosition> self_position = reader.ReadScenePosition();
  const std::optional<std::uint32_t> entity_count =
      reader.ReadIntegral<std::uint32_t>();
  if (!player_id.has_value() || !controlled_entity_id.has_value() ||
      !scene_id.has_value() || !self_position.has_value() ||
      !entity_count.has_value()) {
    return std::nullopt;
  }

  EnterSceneSnapshot snapshot;
  snapshot.player_id = *player_id;
  snapshot.controlled_entity_id = *controlled_entity_id;
  snapshot.scene_id = *scene_id;
  snapshot.self_position = *self_position;
  snapshot.visible_entities.reserve(*entity_count);
  for (std::uint32_t index = 0; index < *entity_count; ++index) {
    const std::optional<VisibleEntitySnapshot> entity =
        reader.ReadVisibleEntitySnapshot();
    if (!entity.has_value()) {
      return std::nullopt;
    }
    snapshot.visible_entities.push_back(*entity);
  }

  if (!reader.consumed_all()) {
    return std::nullopt;
  }
  return snapshot;
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const MoveRequest& message) {
  detail::BufferWriter writer;
  writer.WriteEntityId(message.entity_id);
  writer.WriteScenePosition(message.target_position);
  writer.WriteIntegral(message.client_seq);
  writer.WriteIntegral(message.client_timestamp_ms);
  return writer.TakeBytes();
}

template <>
inline std::optional<MoveRequest> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<EntityId> entity_id = reader.ReadEntityId();
  const std::optional<ScenePosition> target_position = reader.ReadScenePosition();
  const std::optional<std::uint32_t> client_seq =
      reader.ReadIntegral<std::uint32_t>();
  const std::optional<std::uint64_t> client_timestamp_ms =
      reader.ReadIntegral<std::uint64_t>();
  if (!entity_id.has_value() || !target_position.has_value() ||
      !client_seq.has_value() || !client_timestamp_ms.has_value() ||
      !reader.consumed_all()) {
    return std::nullopt;
  }
  return MoveRequest{*entity_id, *target_position, *client_seq, *client_timestamp_ms};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const MoveCorrection& message) {
  detail::BufferWriter writer;
  writer.WriteEntityId(message.entity_id);
  writer.WriteScenePosition(message.authoritative_position);
  writer.WriteIntegral(message.client_seq);
  return writer.TakeBytes();
}

template <>
inline std::optional<MoveCorrection> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<EntityId> entity_id = reader.ReadEntityId();
  const std::optional<ScenePosition> authoritative_position =
      reader.ReadScenePosition();
  const std::optional<std::uint32_t> client_seq =
      reader.ReadIntegral<std::uint32_t>();
  if (!entity_id.has_value() || !authoritative_position.has_value() ||
      !client_seq.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return MoveCorrection{*entity_id, *authoritative_position, *client_seq};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const PickupRequest& message) {
  detail::BufferWriter writer;
  writer.WritePlayerId(message.player_id);
  writer.WriteEntityId(message.drop_entity_id);
  writer.WriteIntegral(message.client_seq);
  return writer.TakeBytes();
}

template <>
inline std::optional<PickupRequest> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  const std::optional<EntityId> drop_entity_id = reader.ReadEntityId();
  const std::optional<std::uint32_t> client_seq =
      reader.ReadIntegral<std::uint32_t>();
  if (!player_id.has_value() || !drop_entity_id.has_value() ||
      !client_seq.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return PickupRequest{*player_id, *drop_entity_id, *client_seq};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const PickupResult& message) {
  detail::BufferWriter writer;
  writer.WritePlayerId(message.player_id);
  writer.WriteEntityId(message.drop_entity_id);
  writer.WriteIntegral(message.error_code);
  writer.WriteIntegral(message.client_seq);
  return writer.TakeBytes();
}

template <>
inline std::optional<PickupResult> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  const std::optional<EntityId> drop_entity_id = reader.ReadEntityId();
  const std::optional<ErrorCode> error_code = reader.ReadIntegral<ErrorCode>();
  const std::optional<std::uint32_t> client_seq =
      reader.ReadIntegral<std::uint32_t>();
  if (!player_id.has_value() || !drop_entity_id.has_value() ||
      !error_code.has_value() || !client_seq.has_value() ||
      !reader.consumed_all()) {
    return std::nullopt;
  }
  return PickupResult{*player_id, *drop_entity_id, *error_code, *client_seq};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const InventoryDelta& message) {
  detail::BufferWriter writer;
  writer.WritePlayerId(message.player_id);
  writer.WriteIntegral<std::uint32_t>(
      static_cast<std::uint32_t>(message.slots.size()));
  for (const InventorySlotDelta& slot : message.slots) {
    writer.WriteInventorySlotDelta(slot);
  }
  return writer.TakeBytes();
}

template <>
inline std::optional<InventoryDelta> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<PlayerId> player_id = reader.ReadPlayerId();
  const std::optional<std::uint32_t> slot_count =
      reader.ReadIntegral<std::uint32_t>();
  if (!player_id.has_value() || !slot_count.has_value()) {
    return std::nullopt;
  }

  InventoryDelta delta;
  delta.player_id = *player_id;
  delta.slots.reserve(*slot_count);
  for (std::uint32_t index = 0; index < *slot_count; ++index) {
    const std::optional<InventorySlotDelta> slot = reader.ReadInventorySlotDelta();
    if (!slot.has_value()) {
      return std::nullopt;
    }
    delta.slots.push_back(*slot);
  }

  if (!reader.consumed_all()) {
    return std::nullopt;
  }
  return delta;
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const SelfState& message) {
  detail::BufferWriter writer;
  writer.WriteEntityId(message.entity_id);
  writer.WriteScenePosition(message.position);
  return writer.TakeBytes();
}

template <>
inline std::optional<SelfState> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<EntityId> entity_id = reader.ReadEntityId();
  const std::optional<ScenePosition> position = reader.ReadScenePosition();
  if (!entity_id.has_value() || !position.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return SelfState{*entity_id, *position};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const AoiEnter& message) {
  detail::BufferWriter writer;
  writer.WriteVisibleEntitySnapshot(message.entity);
  return writer.TakeBytes();
}

template <>
inline std::optional<AoiEnter> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<VisibleEntitySnapshot> entity =
      reader.ReadVisibleEntitySnapshot();
  if (!entity.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return AoiEnter{*entity};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const AoiLeave& message) {
  detail::BufferWriter writer;
  writer.WriteEntityId(message.entity_id);
  return writer.TakeBytes();
}

template <>
inline std::optional<AoiLeave> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<EntityId> entity_id = reader.ReadEntityId();
  if (!entity_id.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return AoiLeave{*entity_id};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const CastSkillRequest& message) {
  detail::BufferWriter writer;
  writer.WriteEntityId(message.caster_entity_id);
  writer.WriteEntityId(message.target_entity_id);
  writer.WriteIntegral(message.skill_id);
  writer.WriteIntegral(message.client_seq);
  return writer.TakeBytes();
}

template <>
inline std::optional<CastSkillRequest> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<EntityId> caster_entity_id = reader.ReadEntityId();
  const std::optional<EntityId> target_entity_id = reader.ReadEntityId();
  const std::optional<std::uint32_t> skill_id =
      reader.ReadIntegral<std::uint32_t>();
  const std::optional<std::uint32_t> client_seq =
      reader.ReadIntegral<std::uint32_t>();
  if (!caster_entity_id.has_value() || !target_entity_id.has_value() ||
      !skill_id.has_value() || !client_seq.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return CastSkillRequest{*caster_entity_id, *target_entity_id, *skill_id,
                          *client_seq};
}

template <>
inline std::vector<std::uint8_t> EncodeMessage(const CastSkillResult& message) {
  detail::BufferWriter writer;
  writer.WriteEntityId(message.caster_entity_id);
  writer.WriteEntityId(message.target_entity_id);
  writer.WriteIntegral(message.skill_id);
  writer.WriteIntegral(message.error_code);
  writer.WriteIntegral(message.hp_delta);
  writer.WriteIntegral(message.client_seq);
  return writer.TakeBytes();
}

template <>
inline std::optional<CastSkillResult> DecodeMessage(
    std::span<const std::uint8_t> payload) {
  detail::BufferReader reader(payload);
  const std::optional<EntityId> caster_entity_id = reader.ReadEntityId();
  const std::optional<EntityId> target_entity_id = reader.ReadEntityId();
  const std::optional<std::uint32_t> skill_id =
      reader.ReadIntegral<std::uint32_t>();
  const std::optional<ErrorCode> error_code = reader.ReadIntegral<ErrorCode>();
  const std::optional<std::int32_t> hp_delta =
      reader.ReadIntegral<std::int32_t>();
  const std::optional<std::uint32_t> client_seq =
      reader.ReadIntegral<std::uint32_t>();
  if (!caster_entity_id.has_value() || !target_entity_id.has_value() ||
      !skill_id.has_value() || !error_code.has_value() || !hp_delta.has_value() ||
      !client_seq.has_value() || !reader.consumed_all()) {
    return std::nullopt;
  }
  return CastSkillResult{*caster_entity_id, *target_entity_id, *skill_id,
                         *error_code, *hp_delta, *client_seq};
}

}  // namespace shared

#endif  // SHARED_PROTOCOL_WIRE_CODEC_H_
