// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_PROPERTIES_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_PROPERTIES_FRAME_H_

#include <ostream>
#include <vector>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"
#include "quic/platform/api/quic_ip_address.h"

namespace quic {

using QuicMcChannelPropertiesContents = uint64_t;
const uint64_t kPropertiesContents_KeyPhase          = UINT64_C(0x0001);
const uint64_t kPropertiesContents_IPFamily          = UINT64_C(0x0002);
const uint64_t kPropertiesContents_UntilPacketNumber = UINT64_C(0x0004);
const uint64_t kPropertiesContents_Addresses         = UINT64_C(0x0008);
const uint64_t kPropertiesContents_SSM               = UINT64_C(0x0010);
const uint64_t kPropertiesContents_HeaderKey         = UINT64_C(0x0020);
const uint64_t kPropertiesContents_Key               = UINT64_C(0x0040);
const uint64_t kPropertiesContents_HashAlgorithm     = UINT64_C(0x0080);
const uint64_t kPropertiesContents_MaxRate           = UINT64_C(0x0100);
const uint64_t kPropertiesContents_MaxIdleTime       = UINT64_C(0x0200);
const uint64_t kPropertiesContents_MaxStreams        = UINT64_C(0x0400);
const uint64_t kPropertiesContents_AckBundleSize     = UINT64_C(0x0800);


class QUIC_EXPORT_PRIVATE QuicMcChannelPropertiesFrame {
 public:
  QuicMcChannelPropertiesFrame() = default;
  QuicMcChannelPropertiesFrame(QuicControlFrameId control_frame_id,
                               QuicChannelPropertiesSequenceNumber channel_properties_sn,
                               QuicChannelId channel_id,
                               QuicPacketCount from_packet_number);

  void SetUntilPacketNumber(QuicPacketCount until_packet_number);
  bool SetSSM(QuicIpAddress source, QuicIpAddress group, uint16_t udp_port);
  bool SetASM(QuicIpAddress group, uint16_t udp_port);

  void SetHeaderKey(uint16_t algorithm, size_t len, const uint8_t* key);
  void SetKey(uint16_t algorithm, bool phase, size_t len, const uint8_t* key);
  void SetHashAlgorithm(uint16_t algorithm);
  void SetMaxRate(uint64_t rate);  // Kibps
  void SetMaxIdleTime(uint64_t duration);  // milliseconds
  void SetMaxStreams(uint64_t max_streams);
  void SetAckBundleSize(uint64_t ack_bundle_size);

  void UnsetUntilPacketNumber() {
    contents &= (~kPropertiesContents_UntilPacketNumber);
  }
  void UnsetAddresses() {
    contents &= (~kPropertiesContents_Addresses);
  }

  void UnsetHeaderKey() {
    contents &= (~kPropertiesContents_HeaderKey);
    header_key.clear();
  }
  void UnsetKey() {
    contents &= (~kPropertiesContents_Key);
    key.clear();
  }
  void UnsetHashAlgorithm() {
    contents &= (~kPropertiesContents_HashAlgorithm);
  }
  void UnsetMaxRate() {
    contents &= (~kPropertiesContents_MaxRate);
  }
  void UnsetMaxidleTime() {
    contents &= (~kPropertiesContents_MaxIdleTime);
  }
  void UnsetMaxStreams() {
    contents &= (~kPropertiesContents_MaxStreams);
  }
  void UnsetAckBundleSize() {
    contents &= (~kPropertiesContents_AckBundleSize);
  }

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcChannelPropertiesFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicChannelPropertiesSequenceNumber channel_properties_sn = 0;
  QuicChannelId channel_id = EmptyQuicChannelId();
  QuicPacketCount from_packet_number = 0;
  QuicMcChannelPropertiesContents contents = 0;

  // optional members
  QuicPacketCount until_packet_number = 0;
  QuicIpAddress source_ip;
  QuicIpAddress group_ip;
  uint16_t udp_port = 0;
  uint16_t header_aead_algorithm = 0;
  std::vector<uint8_t> header_key;
  uint16_t aead_algorithm = 0;
  std::vector<uint8_t> key;
  uint16_t hash_algorithm = 0;
  uint64_t max_rate = 0;  // Kibps
  uint64_t max_idle_time = 0;  // milliseconds
  uint64_t max_streams = 0;
  uint64_t ack_bundle_size = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_PROPERTIES_FRAME_H_
