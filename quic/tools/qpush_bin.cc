// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A binary wrapper for QuicServer.  It listens forever on --port
// (default 6121) until it's killed or ctrl-cd to death.

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <features.h>

#include <vector>
#include <string>

#include "quic/tools/quic_push_commands.h"
#include "quic/platform/api/quic_logging.h"

namespace {

std::string get_strerror(int got_err) {
  enum {
    kMaxStrErrorSize = 256
  };
  char buf[kMaxStrErrorSize];
#ifdef _GNU_SOURCE
  // note: buf may be unused with an immutable static string returned
  char* val = strerror_r(got_err, buf, sizeof(buf));
  return val;
#else
  int rc = strerror_r(got_err, buf, sizeof(buf));
  if (rc != 0) {
    int last_err = errno;
    int len = snprintf(buf, sizeof(buf), "(error in strerror_r=%d: %d)", rc, last_err);
    buf[sizeof(buf)-1]=0;
  }
  return buf;
#endif
}

}  // namespace

int main(int argc, char* argv[]) {
  // TODO: proper command line processing
  // TODO: allow an environment variable instead of passing a path?
  
  /*
  quiche::QuicheSystemEventLoop event_loop("quic_server");
  const char* usage = "Usage: quic_server [options]";
  std::vector<std::string> non_option_args =
      quiche::QuicheParseCommandLineFlags(usage, argc, argv);
  if (!non_option_args.empty()) {
    quiche::QuichePrintCommandLineFlagHelp(usage);
    exit(0);
  }
  */

  std::string top_path_("/tmp/quic-pipes/top-cmd");

  int rc;
  rc = open(top_path_.c_str(), O_RDWR);
  if (rc < 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") opening fifo \"" << top_path_ << "\": " << get_strerror(got_err);
    return -1;
  }
  int fd = rc;

  std::vector<uint8_t> buf;
  buf.resize(16*1024);
  quic::MakePoolCommand pool_cmd(quic::MakePoolCommand::kAlternatives);
  ssize_t consumed = pool_cmd.Encode(buf.size(), &buf[0]);
  if (consumed <= 0) {
    QUIC_LOG(ERROR) << "consumed = " << consumed;
    return -1;
  }
  QUIC_LOG(WARNING) << "sending " << consumed;
  // TODO: loop if not fully written
  rc = write(fd, &buf[0], consumed);
  if (rc < 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") writing to \"" << top_path_ << "\": " << get_strerror(got_err);
    return -1;
  }

  close(fd);
  return 0;
}
