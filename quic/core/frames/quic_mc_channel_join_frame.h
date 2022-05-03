// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_JOIN_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_JOIN_FRAME_H_

#include <ostream>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"

namespace quic {

struct QUIC_EXPORT_PRIVATE QuicMcChannelJoinFrame {
  QuicMcChannelJoinFrame() = default;
  QuicMcChannelJoinFrame(QuicControlFrameId control_frame_id,
                         QuicClientLimitsSequenceNumber limits_sn,
                         QuicClientChannelStateSequenceNumber channel_state_sn,
                         QuicChannelPropertiesSequenceNumber channel_properties_sn,
                         QuicChannelId channel_id);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcChannelJoinFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicClientLimitsSequenceNumber limits_sn = 0;
  QuicClientChannelStateSequenceNumber channel_state_sn = 0;
  QuicChannelPropertiesSequenceNumber channel_properties_sn = 0;
  QuicChannelId channel_id = EmptyQuicChannelId();
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_JOIN_FRAME_H_
