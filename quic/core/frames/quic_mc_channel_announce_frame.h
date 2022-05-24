// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_ANNOUNCE_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_ANNOUNCE_FRAME_H_

#include <ostream>
#include <vector>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"
#include "quic/platform/api/quic_ip_address.h"

namespace quic {

using QuicAEADAlgorithmId = uint16_t;
using QuicHashAlgorithmId = uint16_t;

class QUIC_EXPORT_PRIVATE QuicMcChannelAnnounceFrame {
 public:
  QuicMcChannelAnnounceFrame() = default;
  QuicMcChannelAnnounceFrame(QuicControlFrameId control_frame_id,
                             QuicChannelId channel_id,
                             QuicIpAddress source_ip,
                             QuicIpAddress group_ip,
                             uint16_t udp_port,
                             QuicAEADAlgorithmId header_aead_algorithm,
                             size_t header_key_len,
                             const uint8_t* header_key,
                             QuicAEADAlgorithmId aead_algorithm,
                             QuicHashAlgorithmId hash_algorithm);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcChannelAnnounceFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicChannelId channel_id = EmptyQuicChannelId();
  QuicIpAddress source_ip;
  QuicIpAddress group_ip;
  uint16_t udp_port = 0;
  QuicAEADAlgorithmId header_aead_algorithm = 0;
  std::vector<uint8_t> header_key;
  QuicAEADAlgorithmId aead_algorithm = 0;
  QuicHashAlgorithmId hash_algorithm = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_ANNOUNCE_FRAME_H_
