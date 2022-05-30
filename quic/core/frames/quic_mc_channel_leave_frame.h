// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_LEAVE_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_LEAVE_FRAME_H_

#include <ostream>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"

namespace quic {

struct QUIC_EXPORT_PRIVATE QuicMcChannelLeaveFrame {
  QuicMcChannelLeaveFrame() = default;
  QuicMcChannelLeaveFrame(QuicControlFrameId control_frame_id,
                          QuicChannelId channel_id,
                          QuicClientChannelStateSequenceNumber channel_state_sn,
                          QuicPacketCount after_packet_number);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcChannelLeaveFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicChannelId channel_id = EmptyQuicChannelId();
  QuicClientChannelStateSequenceNumber channel_state_sn = 0;
  QuicPacketCount after_packet_number = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_LEAVE_FRAME_H_
