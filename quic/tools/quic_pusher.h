// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_QUIC_PUSHER_H_
#define QUICHE_QUIC_TOOLS_QUIC_PUSHER_H_

namespace quic {

// A generator of server-initiated unidirectional data
class QuicPusher {
 public:
  virtual ~QuicPusher() = default;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_QUIC_PUSHER_H_
