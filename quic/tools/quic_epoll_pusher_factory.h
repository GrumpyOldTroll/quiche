// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_EPOLL_PUSHER_FACTORY_H_
#define QUICHE_QUIC_TOOLS_EPOLL_PUSHER_FACTORY_H_

#include "quic/platform/api/quic_epoll.h"
#include "quic/tools/quic_pusher.h"
#include "quic/tools/quic_toy_server.h"
#include "quic/tools/quic_epoll_uds_pusher.h"
#include "quic/tools/quic_simple_server_backend.h"

namespace quic {

// Factory creating QuicPusher instances.
class QuicEpollPusherFactory : public QuicToyServer::PusherFactory {
 public:
  explicit QuicEpollPusherFactory(
      QuicEpollServer* epoll_server_);

  std::unique_ptr<QuicPusher> CreatePusher(
      std::string multicast_upstream,
      QuicSpdyServerBase* server,
      QuicSimpleServerBackend* backend) override;
 private:
  QuicEpollServer* epoll_server_;  // unowned
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_EPOLL_PUSHER_FACTORY_H_
