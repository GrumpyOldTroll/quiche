// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_epoll_server_factory.h"

#include <utility>

#include "quic/tools/quic_server.h"

namespace quic {

QuicEpollServerFactory::QuicEpollServerFactory(
    QuicEpollServer* epoll_server): epoll_server_(epoll_server) {}

std::unique_ptr<quic::QuicSpdyServerBase> QuicEpollServerFactory::CreateServer(
    quic::QuicSimpleServerBackend* backend,
    std::unique_ptr<quic::ProofSource> proof_source,
    const quic::ParsedQuicVersionVector& supported_versions) {
  return std::make_unique<quic::QuicServer>(std::move(proof_source), backend,
                                            supported_versions, epoll_server_);
}

}  // namespace quic
