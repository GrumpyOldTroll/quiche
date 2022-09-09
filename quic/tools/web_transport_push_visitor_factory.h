// Copyright (c) 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_WEB_TRANSPORT_PUSH_VISITOR_FACTORY_H_
#define QUICHE_QUIC_TOOLS_WEB_TRANSPORT_PUSH_VISITOR_FACTORY_H_

#include <string>
#include <memory>
#include "quic/tools/web_transport_push_visitors.h"

namespace quic {

class PushWebTransportVisitorFactory
  : public WebTransportVisitorFactory 
{
 public:
  PushWebTransportVisitorFactory(
      std::shared_ptr<QuicPusherPool> pusher_pool,
      std::string path);
  absl::string_view GetPath() const override;
  std::unique_ptr<WebTransportVisitor> Create(
      const spdy::Http2HeaderBlock& request_headers,
      WebTransportSession* session) override;
  ~PushWebTransportVisitorFactory() override = default;
 private:
  std::shared_ptr<QuicPusherPool> pusher_pool_;
  std::string path_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_WEB_TRANSPORT_PUSH_VISITOR_FACTORY_H_
