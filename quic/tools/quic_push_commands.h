// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_PUSH_COMMANDS_H_
#define QUICHE_QUIC_TOOLS_PUSH_COMMANDS_H_

#include <sys/types.h>
#include <stdint.h>
#include <memory>

namespace quic {

class QuicServer;

// consumed is set to the length consumed if a complete command is parsed
// or the negative of the minimum extra we'll need to try again iff
// err is set to kQPP_Bufsize, else it's how much to skip.
// If consumed = 0, the input stream must be abandoned/reset.
bool ParsePushCommand(
    size_t size,
    const uint8_t* buf,
    ssize_t &consumed);

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_PUSH_COMMANDS_H_
