// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_push_commands.h"
#include "quic/tools/quic_server_push_interface.h"
//#include "quic/platform/api/quic_logging.h"

namespace quic {

bool ParsePushCommand(size_t size, const uint8_t* buf, ssize_t& consumed) {
  ServerPushInterface::Top cmd;

  google::protobuf::io::ArrayInputStream input_stream(buf, size);

  bool read_success = ServerPushInterface::readDelimitedFrom(&input_stream, &cmd, &consumed);

  return read_success;
}

}  // namespace quic
