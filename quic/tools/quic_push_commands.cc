// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_push_commands.h"
#include "quic/platform/api/quic_logging.h"

namespace quic {

enum {
  kQuicPushPipeV1Magic = 0x70736831  // 'psh1'
};

ssize_t parse_u32(size_t size, const uint8_t* buf, uint32_t& val) {
  if (size < 4) {
    return size - 4;
  }
  val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  return 4;
}

ssize_t parse_u16(size_t size, const uint8_t* buf, uint16_t& val) {
  if (size < 2) {
    return size - 2;
  }
  val = (buf[0] << 8) | buf[1];
  return 2;
}


std::unique_ptr<QuicPushCommandBase> Empty() {
  return std::unique_ptr<QuicPushCommandBase>();
}

#define CHECK_PARSE(p, s, b, v, c, e) do { \
    ssize_t ux = (p)(s, b, v); \
    if (ux < 0) { \
      (e) = kQPP_Bufsize; \
      (c) = ux; \
      return Empty(); \
    } \
    (s) -= ux; \
    (b) += ux; \
  } while (0)

#define FINISH_PARSE(p, s, b, c, e, o) do { \
    auto fin = (p)(s, b, c, e); \
    if ((e) != kQPP_Bufsize) { \
      (c) += ((o) - (s)); \
    } \
    return fin; \
  } while (0)
    

std::unique_ptr<QuicPushCommandBase> ParsePushCommand(
    size_t size,
    const uint8_t* buf,
    ssize_t &consumed,
    QuicPushParseErrorCode &err) {

  QUIC_LOG(WARNING) << "parsing command sz="<<size;
  consumed = 0;
  err = kQPP_UnknownError;
  size_t start_size = size;
  uint32_t version;
  CHECK_PARSE(parse_u32, size, buf, version, consumed, err);
  if (version != kQuicPushPipeV1Magic) {
    err = kQPP_UnknownVersion;
    return Empty();
  }
  QUIC_LOG(WARNING) << "parsed version sz="<<size;

  uint16_t command;
  CHECK_PARSE(parse_u16, size, buf, command, consumed, err);
  QUIC_LOG(WARNING) << "parsed command " << command << " sz=" << size;

  switch (command) {
    case kQPC_MakePool: {
      FINISH_PARSE(MakePoolCommand::Decode, size, buf, consumed, err, start_size);
    }
    case kQPC_Reserved:
    default: {
      err = kQPP_UnknownCommand;
      return Empty();
    }
  }
}

ssize_t encode_u32(size_t len, uint8_t* buf, uint32_t val) {
  if (len < 4) {
    return len - 4;
  }
  buf[0] = (val & 0xff000000) >> 24;
  buf[1] = (val & 0xff0000) >> 16;
  buf[2] = (val & 0xff00) >> 8;
  buf[3] = (val & 0xff);
  return 4;
}

ssize_t encode_u16(size_t len, uint8_t* buf, uint16_t val) {
  if (len < 2) {
    return len - 2;
  }
  buf[0] = (val & 0xff00) >> 8;
  buf[1] = (val & 0xff);
  return 2;
}

#define CHECK_ENCODE(u, l, b, v) do { \
    ssize_t ux = (u)(l, b, v); \
    if (ux < 0) { \
      return ux; \
    } \
    (l) -= ux; \
    (b) += ux; \
  } while (0)

ssize_t encode_command(size_t len, uint8_t* buf, QuicPushCommandValues cmd) {
  size_t start_len = len;
  CHECK_ENCODE(encode_u32, len, buf, kQuicPushPipeV1Magic);
  CHECK_ENCODE(encode_u16, len, buf, cmd);
  return start_len - len;
}

MakePoolCommand::MakePoolCommand(PoolType pool_type)
    : pool_type_(pool_type) {}

ssize_t MakePoolCommand::Encode(size_t len, uint8_t* buf) {
  size_t start_len = len;
  CHECK_ENCODE(encode_command, len, buf, kQPC_MakePool);
  CHECK_ENCODE(encode_u16, len, buf, pool_type_);
  return start_len - len;
}

std::unique_ptr<QuicPushCommandBase> MakePoolCommand::Decode(
    size_t size,
    const uint8_t* buf,
    ssize_t &consumed,
    QuicPushParseErrorCode &err) {
  size_t start_size = size;
  uint16_t pool_type;
  CHECK_PARSE(parse_u16, size, buf, pool_type, consumed, err);
  QUIC_LOG(WARNING) << "parsed MakePool " << pool_type << " sz=" << size;
  consumed = start_size - size;
  switch (pool_type) {
    case kAlternatives:
    case kOrderedLayers:
    case kArbitraryLayers: {
      err = kQPP_NoError;
      QUIC_LOG(WARNING) << "returned a pool";
      return std::make_unique<MakePoolCommand>(PoolType(pool_type));
    }
    default: {
      QUIC_LOG(ERROR) << "pool bad parameter";
      err = kQPP_BadParameter;
      return Empty();
    }
  }
}

bool MakePoolCommand::RunCommand(CommandArgs& args) {
  QUIC_LOG(WARNING) << "MakePoolCommand::RunCommand";
  return false;
}

}  // namespace quic
