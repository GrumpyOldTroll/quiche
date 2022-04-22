// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_epoll_pusher_factory.h"

#include <utility>

#include "quic/tools/quic_epoll_pipe_pusher.h"
#include "quic/tools/quic_pusher.h"
#include "quic/platform/api/quic_logging.h"

namespace quic {

std::unique_ptr<quic::QuicPusher> QuicEpollPusherFactory::CreatePusher(
    std::string multicast_upstream,
    QuicSpdyServerBase* server) {
  if (multicast_upstream.empty()) {
    return std::unique_ptr<QuicPusher>();
  }
  if (multicast_upstream.substr(0,5) == std::string("root:")) {
    auto pusher = std::make_unique<QuicEpollPipePusher>(
        multicast_upstream.substr(5),
        server);
    if (!pusher->Initialize()) {
      return std::unique_ptr<QuicPusher>();
    }
    return pusher;
  } else {
    QUIC_LOG(ERROR) << "multicast_upstream not yet supported for host:port--use --quic_multicast_upstream=root:<path> instead.\n";
  }
  return std::unique_ptr<QuicPusher>();
}

}  // namespace quic
