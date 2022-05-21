// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_channel_properties_frame.h"

namespace quic {

QuicMcChannelPropertiesFrame::QuicMcChannelPropertiesFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id,
    QuicChannelPropertiesSequenceNumber channel_properties_sn,
    QuicPacketCount from_packet_number,
    QuicPacketCount until_packet_number,
    size_t key_len,
    const uint8_t* key,
    uint64_t max_rate,
    uint64_t max_idle_time,
    uint64_t ack_bundle_size)
    : control_frame_id(control_frame_id),
      channel_id(channel_id),
      channel_properties_sn(channel_properties_sn),
      from_packet_number(from_packet_number),
      until_packet_number(until_packet_number),
      key(key, key+key_len),
      max_rate(max_rate),
      max_idle_time(max_idle_time),
      ack_bundle_size(ack_bundle_size) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcChannelPropertiesFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << ", channel_properties_sn: " << frame.channel_properties_sn
     << ", from_packet_number: " << frame.from_packet_number
     << ", until_packet_number: " << frame.until_packet_number
     << ", key_len: " << frame.key.size()
     << ", max_rate: " << frame.max_rate
     << ", max_idle_time: " << frame.max_idle_time
     << ", ack_bundle_size: " << frame.ack_bundle_size
     << " }\n";
  return os;
}

}  // namespace quic
