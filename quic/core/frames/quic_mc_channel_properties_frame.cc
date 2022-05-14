// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_channel_properties_frame.h"
#include "quic/platform/api/quic_ip_address_family.h"

namespace quic {

QuicMcChannelPropertiesFrame::QuicMcChannelPropertiesFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelPropertiesSequenceNumber channel_properties_sn,
    QuicChannelId channel_id,
    QuicPacketCount from_packet_number)
    : control_frame_id(control_frame_id),
      channel_properties_sn(channel_properties_sn),
      channel_id(channel_id),
      from_packet_number(from_packet_number) {
}

void QuicMcChannelPropertiesFrame::SetUntilPacketNumber(QuicPacketCount in_until_packet_number) {
  until_packet_number = in_until_packet_number;
  contents |= kPropertiesContents_UntilPacketNumber;
}

bool QuicMcChannelPropertiesFrame::SetSSM(QuicIpAddress in_source_ip, QuicIpAddress in_group_ip, uint16_t in_udp_port) {
  if (in_source_ip.address_family() != in_group_ip.address_family()) {
    return false;
  }
  switch(in_group_ip.address_family()) {
    case IpAddressFamily::IP_V4:
      contents &= (~kPropertiesContents_IPFamily);
      break;
    case IpAddressFamily::IP_V6:
      contents |= kPropertiesContents_IPFamily;
      break;
    case IpAddressFamily::IP_UNSPEC:
      return false;
  }
  udp_port = in_udp_port;
  source_ip = in_source_ip;
  group_ip = in_group_ip;
  contents |= (kPropertiesContents_Addresses | kPropertiesContents_SSM);
  return true;
}

bool QuicMcChannelPropertiesFrame::SetASM(QuicIpAddress in_group_ip, uint16_t in_udp_port) {
  switch(in_group_ip.address_family()) {
    case IpAddressFamily::IP_V4:
      contents &= (~kPropertiesContents_IPFamily);
      break;
    case IpAddressFamily::IP_V6:
      contents |= kPropertiesContents_IPFamily;
      break;
    case IpAddressFamily::IP_UNSPEC:
      return false;
  }
  udp_port = in_udp_port;
  source_ip = QuicIpAddress();
  group_ip = in_group_ip;
  contents |= kPropertiesContents_Addresses;
  contents &= (~kPropertiesContents_SSM);
  return true;
}

void QuicMcChannelPropertiesFrame::SetHeaderKey(uint16_t in_algorithm, size_t in_len, const uint8_t* in_key) {
  header_aead_algorithm = in_algorithm;
  std::vector<uint8_t> new_key(in_key, in_key+in_len);
  header_key.swap(new_key);
  contents |= kPropertiesContents_HeaderKey;
}

void QuicMcChannelPropertiesFrame::SetKey(uint16_t in_algorithm, bool in_key_phase, size_t in_len, const uint8_t* in_key) {
  aead_algorithm = in_algorithm;
  std::vector<uint8_t> new_key(in_key, in_key+in_len);
  key.swap(new_key);
  contents |= kPropertiesContents_Key;
  if (in_key_phase) {
    contents |= kPropertiesContents_KeyPhase;
  } else {
    contents &= (~kPropertiesContents_KeyPhase);
  }
}

void QuicMcChannelPropertiesFrame::SetHashAlgorithm(uint16_t in_algorithm) {
  hash_algorithm = in_algorithm;
  contents |= kPropertiesContents_HashAlgorithm;
}

void QuicMcChannelPropertiesFrame::SetMaxRate(uint64_t in_rate) {
  max_rate = in_rate;
  contents |= kPropertiesContents_MaxRate;
}

void QuicMcChannelPropertiesFrame::SetMaxIdleTime(uint64_t in_duration) {
  max_idle_time = in_duration;
  contents |= kPropertiesContents_MaxIdleTime;
}

void QuicMcChannelPropertiesFrame::SetMaxStreams(uint64_t in_max_streams) {
  max_streams = in_max_streams;
  contents |= kPropertiesContents_MaxStreams;
}

void QuicMcChannelPropertiesFrame::SetAckBundleSize(uint64_t in_ack_bundle_size) {
  ack_bundle_size = in_ack_bundle_size;
  contents |= kPropertiesContents_AckBundleSize;
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcChannelPropertiesFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_properties_sn: " << frame.channel_properties_sn
     << ", channel_id: " << frame.channel_id
     << ", from_packet_number: " << frame.from_packet_number
     << ", contents: " << frame.contents;
  if (frame.contents & kPropertiesContents_UntilPacketNumber) {
    os << ", until_packet_number: " << frame.until_packet_number;
  } else {
    os << ", until_packet_number: None";
  }
  if (frame.contents & kPropertiesContents_Addresses) {
    if (frame.contents & kPropertiesContents_SSM) {
      os << ", source_ip: " << frame.source_ip;
    } else {
      os << ", source_ip: None";
    }
    os << ", group_ip: " << frame.group_ip;
  } else {
    os << ", source_ip: None, group_ip: None";
  }
  if (frame.contents & kPropertiesContents_HeaderKey) {
    os << ", header_key_len: " << frame.header_key.size();
  } else {
    os << ", header_key_len: None";
  }
  if (frame.contents & kPropertiesContents_Key) {
    os << ", key_len: " << frame.key.size();
    if (frame.contents & kPropertiesContents_KeyPhase) {
      os << ", key_phase: 1";
    } else {
      os << ", key_phase: 0";
    }
  } else {
    os << ", key_len: None, key_phase: None";
  }
  if (frame.contents & kPropertiesContents_HashAlgorithm) {
    os << ", hash_algorithm: " << frame.hash_algorithm;
  } else {
    os << ", hash_algorithm: None";
  }
  if (frame.contents & kPropertiesContents_MaxRate) {
    os << ", max_rate: " << frame.max_rate;
  } else {
    os << ", max_rate: None";
  }
  if (frame.contents & kPropertiesContents_MaxIdleTime) {
    os << ", max_idle_time: " << frame.max_idle_time;
  } else {
    os << ", max_idle_time: None";
  }
  if (frame.contents & kPropertiesContents_MaxStreams) {
    os << ", max_streams: " << frame.max_streams;
  } else {
    os << ", max_streams: None";
  }
  if (frame.contents & kPropertiesContents_AckBundleSize) {
    os << ", ack_bundle_size: " << frame.ack_bundle_size;
  } else {
    os << ", ack_bundle_size: None";
  }
  os << " }\n";
  return os;
}

}  // namespace quic
