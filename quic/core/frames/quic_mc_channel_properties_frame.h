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
#include "quic/core/frames/quic_mc_channel_announce_frame.h"
#include "quic/core/quic_types.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicMcChannelPropertiesFrame {
 public:
  QuicMcChannelPropertiesFrame() = default;
  QuicMcChannelPropertiesFrame(QuicControlFrameId control_frame_id,
                               QuicChannelId channel_id,
                               QuicChannelPropertiesSequenceNumber channel_properties_sn,
                               QuicPacketCount from_packet_number,
                               QuicPacketCount until_packet_number,
                               size_t key_len,
                               const uint8_t* key,
                               uint64_t max_rate,
                               uint64_t max_idle_time,
                               uint64_t ack_bundle_size);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcChannelPropertiesFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicChannelId channel_id = EmptyQuicChannelId();
  QuicChannelPropertiesSequenceNumber channel_properties_sn = 0;
  QuicPacketCount from_packet_number = 0;
  QuicPacketCount until_packet_number = 0;
  std::vector<uint8_t> key;
  uint64_t max_rate = 0;  // Kibps
  uint64_t max_idle_time = 0;  // milliseconds
  uint64_t ack_bundle_size = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_PROPERTIES_FRAME_H_
