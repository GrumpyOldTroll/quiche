// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_EPOLL_SERVER_FACTORY_H_
#define QUICHE_QUIC_TOOLS_EPOLL_SERVER_FACTORY_H_

#include "quic/platform/api/quic_epoll.h"
#include "quic/tools/quic_toy_server.h"

namespace quic {

// Factory creating QuicServer instances.
class QuicEpollServerFactory : public QuicToyServer::ServerFactory {
 public:
  explicit QuicEpollServerFactory(
      QuicEpollServer* epoll_server);

  std::unique_ptr<QuicSpdyServerBase> CreateServer(
      QuicSimpleServerBackend* backend,
      std::unique_ptr<ProofSource> proof_source,
      const quic::ParsedQuicVersionVector& supported_versions) override;

 private:
  QuicEpollServer *epoll_server_;  // unowned
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_EPOLL_SERVER_FACTORY_H_
