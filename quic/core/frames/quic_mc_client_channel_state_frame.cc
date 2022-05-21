// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_client_channel_state_frame.h"

namespace quic {

QuicMcClientChannelStateFrame::QuicMcClientChannelStateFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id,
    QuicClientChannelStateSequenceNumber channel_state_sn,
    QuicClientChannelStateState state,
    QuicClientChannelStateLeaveReason reason)
    : control_frame_id(control_frame_id),
      channel_id(channel_id),
      channel_state_sn(channel_state_sn),
      state(state),
      reason(reason) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcClientChannelStateFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << ", channel_state_sn: " << frame.channel_state_sn
     << ", state: " << frame.state
     << ", reason: " << frame.reason
     << " }\n";
  return os;
}

}  // namespace quic

