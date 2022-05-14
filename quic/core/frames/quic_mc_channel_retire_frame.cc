// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_mc_channel_retire_frame.h"

namespace quic {

QuicMcChannelRetireFrame::QuicMcChannelRetireFrame(
    QuicControlFrameId control_frame_id,
    QuicChannelId channel_id)
    : control_frame_id(control_frame_id),
      channel_id(channel_id) {
}

std::ostream& operator<<(std::ostream& os,
                         const QuicMcChannelRetireFrame& frame) {
  os << "{ control_frame_id: " << frame.control_frame_id
     << ", channel_id: " << frame.channel_id
     << " }\n";
  return os;
}

}  // namespace quic
