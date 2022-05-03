// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_channel_leave_frame.h"

namespace quic {

QuicMcChannelLeaveFrame::QuicMcChannelLeaveFrame(
    QuicControlFrameId control_frame_id,
    QuicClientChannelStateSequenceNumber channel_state_sn,
    QuicChannelId channel_id,
    QuicPacketCount after_packet_number)
    : control_frame_id(control_frame_id),
      channel_state_sn(channel_state_sn),
      channel_id(channel_id),
      after_packet_number(after_packet_number) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcChannelLeaveFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_state_sn: " << frame.channel_state_sn
     << ", channel_id: " << frame.channel_id
     << ", after_packet_number: " << frame.after_packet_number
     << " }\n";
  return os;
}

}  // namespace quic
