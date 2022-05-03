// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_RETIRE_FRAME_H_
#define QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_RETIRE_FRAME_H_

#include <ostream>

#include "quic/core/quic_channel_id.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_error_codes.h"
#include "quic/core/quic_types.h"

namespace quic {

struct QUIC_EXPORT_PRIVATE QuicMcChannelRetireFrame {
  QuicMcChannelRetireFrame() = default;
  QuicMcChannelRetireFrame(QuicControlFrameId control_frame_id,
                           QuicChannelId channel_id);

  friend QUIC_EXPORT_PRIVATE std::ostream& operator<<(
      std::ostream& os,
      const QuicMcChannelRetireFrame& frame);

  // A unique identifier of this control frame. 0 when this frame is received,
  // and non-zero when sent.
  QuicControlFrameId control_frame_id = kInvalidControlFrameId;
  QuicChannelId channel_id = EmptyQuicChannelId();
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_FRAMES_QUIC_MC_CHANNEL_RETIRE_FRAME_H_
