// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CLIENT_LIMITS_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CLIENT_LIMITS_FRAME_H_

#include <ostream>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"

namespace quic {

using QuicMcClientLimitsCapabilitiesField = uint64_t;
const uint64_t kClientLimits_IPv4 = UINT64_C(0x0001);
const uint64_t kClientLimits_IPv6 = UINT64_C(0x0002);
const uint64_t kClientLimits_SSM  = UINT64_C(0x0004);
const uint64_t kClientLimits_ASM  = UINT64_C(0x0008);

struct QUIC_EXPORT_PRIVATE QuicMcClientLimitsFrame {
  QuicMcClientLimitsFrame() = default;
  QuicMcClientLimitsFrame(QuicControlFrameId control_frame_id,
      QuicClientLimitsSequenceNumber client_limits_sn,
      bool ip4_support,
      bool ip6_support,
      bool ssm_support,
      bool asm_support,
      QuicBitrate max_aggregate_rate,
      QuicChannelCount max_channel_ids,
      QuicChannelCount max_joined);

  QuicMcClientLimitsFrame(QuicControlFrameId control_frame_id,
      QuicClientLimitsSequenceNumber client_limits_sn,
      QuicMcClientLimitsCapabilitiesField capabilities,
      QuicBitrate max_aggregate_rate,
      QuicChannelCount max_channel_ids,
      QuicChannelCount max_joined);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcClientLimitsFrame& frame);

  bool IPv4Support() const {
    return static_cast<bool>(capabilities & kClientLimits_IPv4);
  }

  bool IPv6Support() const {
    return capabilities & kClientLimits_IPv6;
  }

  bool SSMSupport() const {
    return capabilities & kClientLimits_SSM;
  }

  bool ASMSupport() const {
    return capabilities & kClientLimits_ASM;
  }

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicClientLimitsSequenceNumber client_limits_sn = 0;
  QuicMcClientLimitsCapabilitiesField capabilities = 0;
  QuicBitrate max_aggregate_rate = 0;
  QuicChannelCount max_channel_ids = 0;
  QuicChannelCount max_joined = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CLIENT_LIMITS_FRAME_H_
