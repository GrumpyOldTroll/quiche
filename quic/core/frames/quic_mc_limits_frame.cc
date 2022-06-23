// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_limits_frame.h"

namespace quic {

QuicMcLimitsFrame::QuicMcLimitsFrame(
    QuicControlFrameId control_frame_id,
    QuicClientLimitsSequenceNumber client_limits_sn,
    QuicMcLimitsCapabilitiesField capabilities,
    QuicBitrate max_aggregate_rate,
    QuicChannelCount max_channel_ids,
    QuicChannelCount max_joined)
    : control_frame_id(control_frame_id),
      client_limits_sn(client_limits_sn),
      capabilities(capabilities),
      max_aggregate_rate(max_aggregate_rate),
      max_channel_ids(max_channel_ids),
      max_joined(max_joined) {
}

QuicMcLimitsFrame::QuicMcLimitsFrame(
    QuicControlFrameId control_frame_id,
    QuicClientLimitsSequenceNumber client_limits_sn,
    bool ip4_support,
    bool ip6_support,
    bool ssm_support,
    bool asm_support,
    QuicBitrate max_aggregate_rate,
    QuicChannelCount max_channel_ids,
    QuicChannelCount max_joined)
    : QuicMcLimitsFrame(
        control_frame_id,
        (ip4_support ? kClientLimits_IPv4 : 0) |
        (ip6_support ? kClientLimits_IPv6 : 0) |
        (ssm_support ? kClientLimits_SSM : 0) |
        (asm_support ? kClientLimits_ASM : 0),
        client_limits_sn,
        max_aggregate_rate,
        max_channel_ids,
        max_joined) {
}



std::ostream& operator<<(std::ostream& os,
                         const QuicMcLimitsFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", capabilities: " << frame.capabilities
     << ", max_aggregate_rate: " << frame.max_aggregate_rate
     << ", max_channel_ids: " << frame.max_channel_ids
     << ", max_joined: " << frame.max_joined
     << " }\n";
  return os;
}

}  // namespace quic

