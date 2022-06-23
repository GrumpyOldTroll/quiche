// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_join_frame.h"

namespace quic {

QuicMcJoinFrame::QuicMcJoinFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id,
    QuicClientLimitsSequenceNumber limits_sn,
    QuicClientChannelStateSequenceNumber channel_state_sn,
    QuicChannelKeySequenceNumber channel_key_sn)
    : control_frame_id(control_frame_id),
      channel_id(channel_id),
      limits_sn(limits_sn),
      channel_state_sn(channel_state_sn),
      channel_key_sn(channel_key_sn) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcJoinFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << ", limits_sn: " << frame.limits_sn
     << ", channel_state_sn: " << frame.channel_state_sn
     << ", channel_key_sn: " << frame.channel_key_sn
     << " }\n";
  return os;
}

}  // namespace quic
