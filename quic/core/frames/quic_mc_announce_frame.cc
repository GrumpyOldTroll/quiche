// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_announce_frame.h"
#include "quic/platform/api/quic_ip_address_family.h"

namespace quic {

QuicMcAnnounceFrame::QuicMcAnnounceFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id,
    QuicIpAddress source_ip,
    QuicIpAddress group_ip,
    uint16_t udp_port,
    QuicAEADAlgorithmId header_aead_algorithm,
    size_t header_key_len,
    const uint8_t* header_key,
    QuicAEADAlgorithmId aead_algorithm,
    QuicHashAlgorithmId hash_algorithm,
    uint64_t max_rate,
    uint64_t max_idle_time,
    uint64_t ack_bundle_size)
    : control_frame_id(control_frame_id),
      channel_id(channel_id),
      source_ip(source_ip),
      group_ip(group_ip),
      udp_port(udp_port),
      header_aead_algorithm(header_aead_algorithm),
      header_key(header_key, header_key+header_key_len),
      aead_algorithm(aead_algorithm),
      hash_algorithm(hash_algorithm),
      max_rate(max_rate),
      max_idle_time(max_idle_time),
      ack_bundle_size(ack_bundle_size) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcAnnounceFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << ", source_ip: " << frame.source_ip
     << ", group_ip: " << frame.group_ip
     << ", udp_port: " << frame.udp_port
     << ", header_aead_algorithm: " << frame.header_aead_algorithm
     << ", header_key_len: " << frame.header_key.size()
     << ", aead_algorithm: " << frame.aead_algorithm
     << ", hash_algorithm: " << frame.hash_algorithm
     << ", max_rate: " << frame.max_rate
     << ", max_idle_time: " << frame.max_idle_time
     << ", ack_bundle_size: " << frame.ack_bundle_size
     << " }\n";
  return os;
}

}  // namespace quic

