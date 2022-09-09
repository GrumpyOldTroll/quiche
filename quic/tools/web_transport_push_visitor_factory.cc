// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "quic/tools/web_transport_push_visitor_factory.h"
#include "quic/tools/web_transport_push_visitors.h"
#include "quic/tools/quic_pusher.h"

namespace quic {

PushWebTransportVisitorFactory::PushWebTransportVisitorFactory(
    std::shared_ptr<QuicPusherPool> pusher_pool,
    std::string path)
  : pusher_pool_(std::move(pusher_pool)),
    path_(std::move(path))
{}

absl::string_view PushWebTransportVisitorFactory::GetPath() const {
  return absl::string_view(path_);
}

std::unique_ptr<WebTransportVisitor> PushWebTransportVisitorFactory::Create(
    const spdy::Http2HeaderBlock& request_headers,
    WebTransportSession* session) {
  return std::make_unique<PushWebTransportSessionVisitor>(
      pusher_pool_.get(), session);
}

}  // namespace quic
