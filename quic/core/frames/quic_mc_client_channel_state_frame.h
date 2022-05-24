// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CLIENT_CHANNEL_STATE_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CLIENT_CHANNEL_STATE_FRAME_H_

#include <ostream>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"

namespace quic {

using QuicClientChannelStateSequenceNumber = uint64_t;
enum QuicClientChannelStateState {
  kQuicClientChannelStateState_Invalid = 0,
  kQuicClientChannelStateState_Join = 1,
  kQuicClientChannelStateState_DeclinedJoin = 2,
  kQuicClientChannelStateState_Left = 3
};
enum QuicClientChannelStateReasonConstants {
  kQuicClientChannelStateReason_Unspecified = 0,
  kQuicClientChannelStateReason_ServerRequested = 1,
  kQuicClientChannelStateReason_AdministrativeBlock = 2,
  kQuicClientChannelStateReason_ProtocolError = 3,
  kQuicClientChannelStateReason_PropertyViolation = 4,
  kQuicClientChannelStateReason_UnsynchronizedProperties = 5,
  kQuicClientChannelStateReason_IdCollision = 6,
  kQuicClientChannelStateReason_MaxIdleTimeExceeded = 0x11,
  kQuicClientChannelStateReason_MaxRateExceeded = 0x12,
  kQuicClientChannelStateReason_HighLoss = 0x13,
  kQuicClientChannelStateReason_MinAppSpecific = 0x1000000,
  kQuicClientChannelStateReason_MaxAppSpecific = 0x3fffffff,
};
using QuicClientChannelStateLeaveReason = uint64_t;

struct QUIC_EXPORT_PRIVATE QuicMcClientChannelStateFrame {
  QuicMcClientChannelStateFrame() = default;
  QuicMcClientChannelStateFrame(QuicControlFrameId control_frame_id,
                                QuicChannelId channel_id,
                                QuicClientChannelStateSequenceNumber channel_state_sn,
                                QuicClientChannelStateState state,
                                QuicClientChannelStateLeaveReason reason);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcClientChannelStateFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicChannelId channel_id = EmptyQuicChannelId();
  QuicClientChannelStateSequenceNumber channel_state_sn = 0;
  QuicClientChannelStateState state = kQuicClientChannelStateState_Invalid;
  QuicClientChannelStateLeaveReason reason = kQuicClientChannelStateReason_Unspecified;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CLIENT_CHANNEL_STATE_FRAME_H_
