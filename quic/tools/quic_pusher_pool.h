// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_QUIC_PUSHER_POOL_H_
#define QUICHE_QUIC_TOOLS_QUIC_PUSHER_POOL_H_

#include <inttypes.h>
#include "quic/platform/api/quic_logging.h"
#include "absl/strings/string_view.h"

namespace quic {

class WebTransportVisitor;
class PushWebTransportSessionVisitor;

class QuicPusherChannel {
 public:
  virtual ~QuicPusherChannel() {}
  virtual absl::string_view GetTok() const = 0;
  virtual bool SendData(std::shared_ptr<std::string> data) = 0;
};

// A generator of server-initiated unidirectional data
class QuicPusherPool {
 public:
  virtual ~QuicPusherPool() = default;
  virtual void VisitorReady(PushWebTransportSessionVisitor *visitor) {
    QUIC_LOG(WARNING) << "Push Visitor Ready";
  }
  virtual void VisitorStopped(PushWebTransportSessionVisitor *visitor) {
    QUIC_LOG(WARNING) << "Push Visitor Stopped";
  }
  virtual QuicPusherChannel* MakeChannel(
      uint32_t ordinal) = 0;
  virtual QuicPusherChannel* FindChannel(absl::string_view channel_tok) = 0;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_QUIC_PUSHER_POOL_H_
