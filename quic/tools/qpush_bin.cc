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
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>
#include <inttypes.h>

#include <vector>
#include <map>
#include <string>

#include "quic/tools/quic_push_commands.h"
#include "quic/platform/api/quic_logging.h"
#include "quic/core/proto/push_server_proto.h"
#include "net/third_party/quiche/src/quic/tools/quic_push_utils.h"

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

struct argv_ctx {
  int top_argc;
  char** top_argv;
  int verbose = 0;
  std::string sock_path = "/tmp/quic.sock";
  int fd = -1;
  std::vector<uint8_t> buf;
  ssize_t msg_size = 0;
};

}  // namespace

void usage(char** argv) {
  printf("usage: %s -s <sock_path> [-v] <cmd> <args>\n"
      "  -s default: /tmp/quic.sock\n"
      "  -v verbosity (multiple settings stack)\n"
      "<cmd> is one of:\n"
      "  makepool <name> [alt|ordered|arbitrary] [raw|webtrans]\n"
      "     (writes pool_token to stdout on success)\n"
      "  makechannel <pool_token> <ordinal> <source> <group> <port> <rate> <ack_delay>\n"
      "     (writes channel_token to stdout on success)\n"
      "  fullstream <channel_token> <file>\n"
      "     (returns success if data will be pushed to clients in theory)\n"
      , argv[0]);
}

typedef int (*resp_handler)(argv_ctx& cv, const quic::PushCmd_Response& resp);


int makepool_resp(argv_ctx& cv, const quic::PushCmd_Response& resp) {
  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "makepool resp()";
  }
  if (!resp.has_make_pool_resp()) {
    QUIC_LOG(ERROR) << "makepool response without a make_pool_resp";
    return -1;
  }
  const quic::MakePool_Response& subresp = resp.make_pool_resp();
  printf("%s\n", subresp.pool_token().c_str());
  return 0;
}

int makepool_cmd(argv_ctx& cv, int argc, char** argv, resp_handler& rh) {
  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "makepool cmd(" << argc << ", " << argv[0] << "...)";
  }
  cv.buf.resize(16*1024);
  google::protobuf::io::ArrayOutputStream output_stream(&cv.buf[0], cv.buf.size());

  if (argc != 4) {
    usage(cv.top_argv);
    QUIC_LOG(ERROR) << "makepool requires 3 arguments (got " << (argc-1) << ")";
    return -1;
  }
  const char* pool_id = argv[1];
  const char* pool_type = argv[2];
  const char* pool_app = argv[3];

  quic::PushCmd top_cmd;
  quic::MakePool* pool = top_cmd.mutable_make_pool();
  pool->set_pool_id(pool_id);

  if (!strcmp(pool_type, "alt")) {
    pool->set_type(quic::MakePool_Type_Alternatives);
  } else if (!strcmp(pool_type, "ordered")) {
    pool->set_type(quic::MakePool_Type_OrderedLayers);
  } else if (!strcmp(pool_type, "arbitrary")) {
    pool->set_type(quic::MakePool_Type_ArbitraryLayers);
  } else {
    usage(cv.top_argv);
    QUIC_LOG(ERROR) << "unknown pool type " << pool_type;
    return -1;
  }

  if (!strcmp(pool_app, "raw")) {
    pool->set_application(quic::MakePool_Application_Raw);
  } else if (!strcmp(pool_app, "webtrans")) {
    pool->set_application(quic::MakePool_Application_WebTransport);
  } else {
    usage(cv.top_argv);
    QUIC_LOG(ERROR) << "unknown pool app " << pool_app;
    return -1;
  }

  bool write_success =
    quic::PushUtils::writeDelimitedTo(top_cmd, &output_stream);
  cv.msg_size = output_stream.ByteCount();

  if (cv.msg_size <= 0 || !write_success) {
    QUIC_LOG(ERROR) << "msg_size=" << cv.msg_size << ",write_success=" << write_success;
    return -1;
  }

  rh = makepool_resp;

  return 0;
}


int makechannel_resp(argv_ctx& cv, const quic::PushCmd_Response& resp) {
  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "makechannel resp()";
  }
  if (!resp.has_make_channel_resp()) {
    QUIC_LOG(ERROR) << "makechannel response without a make_channel_resp";
    return -1;
  }
  const quic::MakeChannel_Response& subresp = resp.make_channel_resp();
  printf("%s\n", subresp.channel_token().c_str());
  return 0;
}

int makechannel_cmd(argv_ctx& cv, int argc, char** argv, resp_handler& rh) {
  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "makepool cmd(" << argc << ", " << argv[0] << "...)";
  }
  cv.buf.resize(16*1024);
  google::protobuf::io::ArrayOutputStream output_stream(&cv.buf[0], cv.buf.size());

  if (argc != 8) {
    usage(cv.top_argv);
    QUIC_LOG(ERROR) << "makepool requires 7 arguments (got " << (argc-1) << ")";
    return -1;
  }

  const char* pool_token = argv[1];
  const char* ordinal_str = argv[2];
  const char* source = argv[3];
  const char* group = argv[4];
  const char* port_str = argv[5];
  const char* rate_str = argv[6];
  const char* ack_delay_str = argv[7];

  int fail = 0;
  uint16_t port = 0;
  uint64_t rate = 0;
  uint64_t ack_delay = 0;
  uint32_t ordinal = 0;
  int rc;
  rc = sscanf(ordinal_str, "%" PRIu32, &ordinal);
  if (rc != 1) {
    QUIC_LOG(ERROR) << "could not interpret ordinal=" << ordinal_str << " as uint64";
    fail = 1;
  }
  // linux compiler bug?  library bug?
  // this gets: error: format specifies type 'unsigned int *' but the argument has type 'uint16_t *' (aka 'unsigned short *') [-Werror,-Wformat]
  // rc = sscanf(port_str, "%" PRIu16, &port);
  rc = sscanf(port_str, "%hu" , &port);
  if (rc != 1) {
    QUIC_LOG(ERROR) << "could not interpret port=" << port_str << " as uint16";
    fail = 1;
  }
  rc = sscanf(rate_str, "%" PRIu64, &rate);
  if (rc != 1) {
    QUIC_LOG(ERROR) << "could not interpret rate=" << rate_str << " as uint64";
    fail = 1;
  }
  rc = sscanf(ack_delay_str, "%" PRIu64, &ack_delay);
  if (rc != 1) {
    QUIC_LOG(ERROR) << "could not interpret ack_delay=" << ack_delay_str << " as uint64";
    fail = 1;
  }
  // TODO: parse IP addresses and check for same family?
  if (fail) {
    return -1;
  }

  quic::PushCmd top_cmd;
  quic::MakeChannel* chan = top_cmd.mutable_make_channel();
  chan->set_pool_token(pool_token);
  chan->set_pool_ordinal(ordinal);
  chan->set_source_address(source);
  chan->set_group_address(group);
  chan->set_port(port);
  chan->set_max_rate(rate);
  chan->set_max_ack_delay(ack_delay);

  bool write_success =
    quic::PushUtils::writeDelimitedTo(top_cmd, &output_stream);
  cv.msg_size = output_stream.ByteCount();

  if (cv.msg_size <= 0 || !write_success) {
    QUIC_LOG(ERROR) << "msg_size=" << cv.msg_size << ",write_success=" << write_success;
    return -1;
  }

  rh = makechannel_resp;

  return 0;
}


int fullstream_cmd(argv_ctx& cv, int argc, char** argv, resp_handler& rh) {
  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "fullstream cmd(" << argc << ", " << argv[0] << "...)";
  }
  if (argc != 3) {
    usage(cv.top_argv);
    QUIC_LOG(ERROR) << "fullstream requires 2 arguments (got " << (argc-1) << ")";
    return -1;
  }
  const char* channel_token = argv[1];
  const char* file_str = argv[2];

  quic::PushCmd top_cmd;
  quic::FullStream* strm = top_cmd.mutable_full_stream();
  strm->set_channel_token(channel_token);

  struct stat st;
  int rc = stat(file_str, &st);
  if (rc < 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") on stat(" << file_str << "): " << get_strerror(got_err);
    return -1;
  }

  uint32_t max_size = 32*1024;
  if (st.st_size > max_size) {
    QUIC_LOG(ERROR) << "error: " << file_str << " size=" << st.st_size << " exceeds max allowed (" << max_size << ")";
    return -1;
  }
  std::vector<char> buf;
  buf.resize(st.st_size);
  int fd = open(file_str, O_RDONLY);
  if (fd < 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") on open(" << file_str << ",O_RDONLY): " << get_strerror(got_err);
    return -1;
  }
  ssize_t len = read(fd, &buf[0], buf.size());
  if (len < 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") on read(" << file_str << ","  << len << "): " << get_strerror(got_err);
    close(fd);
    return -1;
  }
  if (len < st.st_size) {
    QUIC_LOG(WARNING) << "warning: read only " << len << " bytes, expected " << st.st_size << " from stat";
  }
  close(fd);

  strm->set_data(&buf[0], len);
  cv.buf.resize(len + 500);
  google::protobuf::io::ArrayOutputStream output_stream(&cv.buf[0], cv.buf.size());

  bool write_success =
    quic::PushUtils::writeDelimitedTo(top_cmd, &output_stream);
  cv.msg_size = output_stream.ByteCount();

  if (cv.msg_size <= 0 || !write_success) {
    QUIC_LOG(ERROR) << "msg_size=" << cv.msg_size << ", write failed";
    return -1;
  }

  rh = 0;

  return 0;
}


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

  typedef int (*cmd_fn)(argv_ctx&, int, char**, resp_handler& rh);
  std::map<std::string, cmd_fn> cmd_map;
  cmd_map.insert(std::make_pair(std::string("makepool"), makepool_cmd));
  cmd_map.insert(std::make_pair(std::string("makechannel"), makechannel_cmd));
  cmd_map.insert(std::make_pair(std::string("fullstream"), fullstream_cmd));

  int ch, rc;
  struct argv_ctx cv;
  cv.top_argc = argc;
  cv.top_argv = argv;

  struct option long_opts[] = {
    { "sock", required_argument, 0, 's' },
    { "verbose", no_argument, 0, 'v' },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
  };

  while ((ch =
      getopt_long(argc, argv, "+s:vh", long_opts, NULL)) != -1) {
    switch (ch) {
      case 's': {
        cv.sock_path = std::string(optarg);
        break;
      }
      case 'v': {
        cv.verbose += 1;
        break;
      }
      case 'h': {
        usage(argv);
        return 0;
      }
      case '?': {
        usage(argv);
        QUIC_LOG(ERROR) << "unknown arg \"" << argv[optind-1] << "\"";
        return -1;
      }
      default:
        usage(argv);
        QUIC_LOG(ERROR) << "internal error handling (" << ch << ") on unknown arg \"" << argv[optind-1] << "\"\n";
        return -1;
    }
  }

  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "verbosity=" << cv.verbose
      << ", sock_path=" << cv.sock_path
      << ", next arg=" << optind;
  }

  if (optind >= argc) {
    QUIC_LOG(ERROR) << "missing command\n";
    return -1;
  }
  auto cmd_it = cmd_map.find(std::string(argv[optind]));
  if (cmd_it == cmd_map.end()) {
    QUIC_LOG(ERROR) << "unknown command: " << argv[optind] << "\n";
    return -1;
  }

  resp_handler rh = 0;
  rc = (*cmd_it->second)(cv, argc-optind, &argv[optind], rh);

  if (cv.msg_size <= 0) {
    QUIC_LOG(ERROR) << "msg_size=" << cv.msg_size;
    return -1;
  }
  if (cv.verbose > 0) {
    QUIC_LOG(WARNING) << "msg_size=" << cv.msg_size;

#if 0
    quic::PushServer input_cmd;
    google::protobuf::io::ArrayInputStream input_stream(&cv.buf[0], cv.msg_size);
    ssize_t consumed = 0;
    bool read_success =
        quic::PushUtils::readDelimitedFrom(&input_stream, &input_cmd, &consumed);
    QUIC_LOG(WARNING) << "read_success="<<read_success;
#endif  // 0
  }

  struct sockaddr_un local;

  cv.fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == cv.fd) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") creating socket: " << get_strerror(got_err);
    return -1;
  }

  local.sun_family = AF_UNIX;
  size_t max_pathlen = sizeof(local.sun_path);
  strncpy(local.sun_path, cv.sock_path.c_str(), max_pathlen);
  local.sun_path[max_pathlen-1] = 0;
  size_t sa_len = offsetof(struct sockaddr_un, sun_path) + strlen(local.sun_path) + 1;
  rc = connect(cv.fd, (struct sockaddr*)&local, sa_len);
  if (rc != 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") connecting socket(AF_UNIX," << cv.sock_path << "): " << get_strerror(got_err);
    return -1;
  }

  QUIC_LOG(WARNING) << "sending " << cv.msg_size;
  // TODO: loop if not fully written
  rc = write(cv.fd, &cv.buf[0], cv.msg_size);
  if (rc < 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") writing to \"" << cv.sock_path << "\": " << get_strerror(got_err);
    return -1;
  }

  quic::PushCmd_Response resp;
  std::vector<uint8_t> resp_buf;
  resp_buf.resize(16*1024);
  size_t resp_tot = 0;
  ssize_t consumed = 0;
  ssize_t read_len;
  bool read_success;
  do {
    read_len = read(cv.fd, &resp_buf[resp_tot], resp_buf.size() - resp_tot);
    if (read_len < 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err <<") reading response: " << get_strerror(got_err);
      return -1;
    }
    resp_tot += read_len;
    google::protobuf::io::ArrayInputStream input_stream(&resp_buf[0], resp_tot);
    read_success = quic::PushUtils::readDelimitedFrom(&input_stream,
        &resp, &consumed);
  } while (!read_success);

  if (!resp.success()) {
    QUIC_LOG(ERROR) << "error: response says failure: " << resp.error_message();
    return -1;
  }

  rc = 0;
  if (rh) {
    if (cv.verbose > 0) {
      QUIC_LOG(WARNING) << "handling data-bearing response";
    }
    rc  = rh(cv, resp);
  }

  close(cv.fd);
  return rc;
}
