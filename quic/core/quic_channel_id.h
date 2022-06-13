// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_QUIC_CHANNEL_ID_H_
#define QUICHE_QUIC_CORE_QUIC_CHANNEL_ID_H_

#include "quic/core/quic_connection_id.h"

namespace quic {

using QuicChannelId = QuicConnectionId;
using QuicChannelIdHash = QuicConnectionIdHash;

// Creates a channel id with value 0
QUIC_EXPORT_PRIVATE QuicChannelId EmptyQuicChannelId();

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_QUIC_CHANNEL_ID_H_
