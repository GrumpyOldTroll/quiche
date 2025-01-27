// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_blocked_frame.h"

#include "quic/core/quic_types.h"

namespace quic {

QuicBlockedFrame::QuicBlockedFrame() : QuicInlinedFrame(BLOCKED_FRAME) {}

QuicBlockedFrame::QuicBlockedFrame(QuicControlFrameId control_frame_id,
                                   QuicStreamId stream_id)
    : QuicInlinedFrame(BLOCKED_FRAME),
      control_frame_id(control_frame_id),
      stream_id(stream_id),
      offset(0) {}

QuicBlockedFrame::QuicBlockedFrame(QuicControlFrameId control_frame_id,
                                   QuicStreamId stream_id,
                                   QuicStreamOffset offset)
    : QuicInlinedFrame(BLOCKED_FRAME),
      control_frame_id(control_frame_id),
      stream_id(stream_id),
      offset(offset) {}

std::ostream& operator<<(std::ostream& os,
                         const QuicBlockedFrame& blocked_frame) {
  os << "{ control_frame_id: " << blocked_frame.control_frame_id
     << ", stream_id: " << blocked_frame.stream_id << " }\n";
  return os;
}

}  // namespace quic
