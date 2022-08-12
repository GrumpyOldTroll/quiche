// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_pusher_alternatives.h"
#include "quic/tools/web_transport_push_visitors.h"

namespace quic {

QuicPusherAltChannel::QuicPusherAltChannel(
    uint32_t ordinal)
{
  std::ostringstream str;
  str << ordinal;
  path_ = str.str();
}

absl::string_view QuicPusherAltChannel::GetTok() const {
  return absl::string_view(path_);
}

bool QuicPusherAltChannel::SendData(std::shared_ptr<std::string> data) {
  full_streams_.push_back(data);

  for (auto it = session_visitors_.begin(); it != session_visitors_.end(); ++it) {
    PushWebTransportSessionVisitor *visitor = *it;
    visitor->QueuePushUnidirectionalStream(*data);
  }
  return true;
}

void QuicPusherAlternatives::VisitorReady(PushWebTransportSessionVisitor *visitor) {
  if (!channels_.empty()) {
    QuicPusherAltChannel *first_channel = &channels_.begin()->second;
    first_channel->session_visitors_.push_back(visitor);
  }
}

void QuicPusherAlternatives::VisitorStopped(PushWebTransportSessionVisitor *visitor) {
}

QuicPusherChannel* QuicPusherAlternatives::MakeChannel(
    uint32_t ordinal) {
  auto loc = channels_.find(ordinal);
  if (loc != channels_.end()) {
    return 0;
  }
  auto result = channels_.emplace(ordinal, QuicPusherAltChannel(
        ordinal));
  if (!result.second) {
    return 0;
  }
  return &result.first->second;
}

QuicPusherChannel* QuicPusherAlternatives::FindChannel(absl::string_view tok) {
  uint32_t ordinal = 0;
  int ret = sscanf(&tok[0], "%" PRIu32 "u", &ordinal);
  if (ret != 1) {
    return 0;
  }
  auto loc = channels_.find(ordinal);
  if (loc == channels_.end()) {
    return 0;
  }
  return &loc->second;
}

}  // namespace quic
