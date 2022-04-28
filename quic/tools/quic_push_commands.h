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

class QuicPushCommandBase {
 public:
  virtual ~QuicPushCommandBase() = default;

  struct CommandArgs {
    QuicServer* server;
  };

  // returns the the length consumed if len is big enough
  // or the negative of the amount we're short if len was too small
  virtual ssize_t Encode(size_t len, uint8_t* buf) = 0;
  virtual bool RunCommand(CommandArgs &args) = 0;
};

enum QuicPushParseErrorCode {
  kQPP_NoError = 0,
  kQPP_UnknownError,
  kQPP_Bufsize,
  kQPP_UnknownVersion,
  kQPP_UnknownCommand,
  kQPP_BadParameter,
};

enum QuicPushCommandValues {
  kQPC_Reserved = 0,
  kQPC_MakePool = 1,
};

// consumed is set to the length consumed if a complete command is parsed
// or the negative of the minimum extra we'll need to try again iff
// err is set to kQPP_Bufsize, else it's how much to skip.
// If consumed = 0, the input stream must be abandoned/reset.
std::unique_ptr<QuicPushCommandBase> ParsePushCommand(
    size_t size,
    const uint8_t* buf,
    ssize_t &consumed,
    QuicPushParseErrorCode &err);

class MakePoolCommand : public QuicPushCommandBase {
 public:
  enum PoolType {
    kReserved,
    kAlternatives,
    kOrderedLayers,
    kArbitraryLayers
  };
  explicit MakePoolCommand(PoolType pool_type);
  ssize_t Encode(size_t len, uint8_t* buf) override;
  static std::unique_ptr<QuicPushCommandBase> Decode(
      size_t size,
      const uint8_t* buf,
      ssize_t &consumed,
      QuicPushParseErrorCode &err);

  bool RunCommand(CommandArgs &args) override;

 private:
  PoolType pool_type_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_PUSH_COMMANDS_H_
