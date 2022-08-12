// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_QUIC_PUSHER_ALTERNATIVES_H_
#define QUICHE_QUIC_TOOLS_QUIC_PUSHER_ALTERNATIVES_H_

#include <map>
#include <list>
#include "quic/tools/quic_pusher_pool.h"

namespace quic {

class QuicPusherAlternatives;
class PushWebTransportSessionVisitor;

class QuicPusherAltChannel: public QuicPusherChannel {
 public:
  explicit QuicPusherAltChannel(
      uint32_t ordinal);
  ~QuicPusherAltChannel() override = default;
  absl::string_view GetTok() const override;
  bool SendData(std::shared_ptr<std::string> data) override;
 private:
  std::string path_;
  std::list<std::shared_ptr<std::string> > full_streams_;
  std::list<PushWebTransportSessionVisitor *> session_visitors_;

  friend class QuicPusherAlternatives;
};

// A generator of server-initiated unidirectional data
class QuicPusherAlternatives : public QuicPusherPool {
 public:
  ~QuicPusherAlternatives() override = default;

  void VisitorReady(PushWebTransportSessionVisitor *visitor) override;
  void VisitorStopped(PushWebTransportSessionVisitor *visitor) override;

  QuicPusherChannel* MakeChannel(uint32_t ordinal) override;
  QuicPusherChannel* FindChannel(absl::string_view tok) override;
 private:
  std::map<uint32_t, QuicPusherAltChannel> channels_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_QUIC_PUSHER_ALTERNATIVES_H_
