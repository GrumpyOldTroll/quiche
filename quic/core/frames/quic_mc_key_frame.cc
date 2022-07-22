// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_key_frame.h"

namespace quic {

QuicMcKeyFrame::QuicMcKeyFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id,
    QuicChannelKeySequenceNumber channel_key_sn,
    QuicPacketCount from_packet_number,
    size_t key_len,
    const uint8_t* key)
    : control_frame_id(control_frame_id),
      channel_id(channel_id),
      channel_key_sn(channel_key_sn),
      from_packet_number(from_packet_number),
      key(key, key+key_len) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcKeyFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << ", channel_key_sn: " << frame.channel_key_sn
     << ", from_packet_number: " << frame.from_packet_number
     << ", key_len: " << frame.key.size()
     << " }\n";
  return os;
}

}  // namespace quic
