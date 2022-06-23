// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_leave_frame.h"

namespace quic {

QuicMcLeaveFrame::QuicMcLeaveFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id,
    QuicClientChannelStateSequenceNumber channel_state_sn,
    QuicPacketCount after_packet_number)
    : control_frame_id(control_frame_id),
      channel_id(channel_id),
      channel_state_sn(channel_state_sn),
      after_packet_number(after_packet_number) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcLeaveFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << ", channel_state_sn: " << frame.channel_state_sn
     << ", after_packet_number: " << frame.after_packet_number
     << " }\n";
  return os;
}

}  // namespace quic
