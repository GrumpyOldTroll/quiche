// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_epoll_uds_pusher.h"

#include <errno.h>
#include <features.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <algorithm>
#include <utility>
#include <sstream>

#include "quic/platform/api/quic_logging.h"
#include "quic/tools/quic_server.h"

namespace {

enum {
  kDefaultReadBufsize = 1024 * 16,
};

std::string get_strerror(int got_err) {
  enum { kMaxStrErrorSize = 256 };
  char buf[kMaxStrErrorSize];
#ifdef _GNU_SOURCE
  // note: buf may be unused with an immutable static string returned
  char* val = strerror_r(got_err, buf, sizeof(buf));
  return val;
#else
  int rc = strerror_r(got_err, buf, sizeof(buf));
  if (rc != 0) {
    int last_err = errno;
    int len = snprintf(buf, sizeof(buf), "(error in strerror_r=%d: %d)", rc,
                       last_err);
    buf[sizeof(buf) - 1] = 0;
  }
  return buf;
#endif
}

/*
std::size_t stringstream_buffersize(const std::stringstream& ss) {
  std::streambuf* buf = ss.rdbuf();
  std::streampos pos = buf->pubseekoff(0, std::ios_base::cur,
std::ios_base::in); std::streampos end = buf->pubseekoff(0, std::ios_base::end,
std::ios_base::in); buf->pubseekpos(pos, std::ios_base::in); return end - pos;
}
*/

}  // namespace

namespace quic {

QuicEpollUDSPusher::QuicEpollUDSPusher(std::string socket_path,
                                       QuicEpollServer* epoll_server,
                                       QuicSpdyServerBase* server)
    : path_(socket_path),
      topcmd_fd_(-1),
      epoll_server_(epoll_server),
      server_(server),
      event_count_(0),
      max_incoming_connections_(30) {
        std::cout << "UDS Pusher" << std::endl;
      }

QuicEpollUDSPusher::~QuicEpollUDSPusher() {
  Shutdown();
}

bool QuicEpollUDSPusher::Initialize() {
  QUIC_LOG(INFO) << "Initialize(" << path_ << ")";

  int s = 0;
  struct sockaddr_un local;

  s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == s) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") creating socket: " << get_strerror(got_err);
    return false;
  }

  local.sun_family = AF_UNIX;
  size_t max_pathlen = sizeof(local.sun_path);
  strncpy(local.sun_path, path_.c_str(), max_pathlen);
  local.sun_path[max_pathlen-1] = 0;
  unlink(local.sun_path);

  size_t sa_len = offsetof(struct sockaddr_un, sun_path) + strlen(local.sun_path) + 1;
  if (bind(s, (struct sockaddr*)&local, sa_len) != 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") binding socket: " << get_strerror(got_err);
    return false;
  }

  if (listen(s, max_incoming_connections_) != 0) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") listening: " << get_strerror(got_err);
    return false;
  }

  topcmd_fd_ = s;
  epoll_server_->RegisterFDForRead(topcmd_fd_, this);

  return true;
}

void QuicEpollUDSPusher::Shutdown() {
  QUIC_LOG(WARNING) << "Shutdown(" << path_ << ")";
  if (topcmd_fd_ != -1) {
    epoll_server_->UnregisterFD(topcmd_fd_);
    int rc;
    rc = close(topcmd_fd_);
    if (rc != 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err << ") closing \"" << path_
                      << "\": " << get_strerror(got_err);
    }
    topcmd_fd_ = -1;
    rc = unlink(path_.c_str());
    if (rc != 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err << ") unlinking \"" << path_
                      << "\": " << get_strerror(got_err);
    }
  }
  while (!clients_.empty()) {
    Client& client = *clients_.begin()->second;
    CloseClient(client);
  }
}

bool QuicEpollUDSPusher::RunBufferedCommands(QuicEpollUDSPusher::Client& client) {

  auto& filled_len = client.filled_len;
  auto& client_buffer = client.buffer;

  enum { kCommandLimit = 10 };
  int commands = 0;
  // TODO: maybe a time limit is better than a command count limit?  Or
  //       do we need both?  --jake 2022-04-24

  size_t consumed = 0;
  ssize_t just_ate = 0;
  QuicPushParseErrorCode err = kQPP_NoError;
  QuicPushCommandBase::CommandArgs args;
  args.server = (QuicServer*)server_;
  while (consumed < filled_len && err == kQPP_NoError &&
         commands < kCommandLimit) {
    just_ate = 0;

    quic::PushCmd push_cmd;

    auto read_success = parseCommand(client, push_cmd,
        client_buffer.size() - consumed, &client_buffer[consumed],
        just_ate, err);
    if (!read_success) {
      QUIC_LOG(WARNING) << "no command parsed from " << filled_len
                        << " bytes at offset " << consumed << " (" << just_ate
                        << " output as consumed)";
      CloseClient(client);
      return false;
    }

    QUIC_LOG(WARNING) << "received command " << push_cmd.DebugString();

    QUIC_LOG(WARNING) << "command parsed from " << filled_len
                      << " bytes at offset " << consumed << " consuming "
                      << just_ate;
    // assert(just_ate > 0);
    consumed += just_ate;
    commands += 1;

    quic::PushCmd_Response cmd_resp;
    RunCommand(push_cmd, cmd_resp);
    client.response_buf.resize(16*1024);
    google::protobuf::io::ArrayOutputStream output_stream(
        &client.response_buf[client.response_len],
        client.response_buf.size() - client.response_len);
    bool resp_write_success = quic::PushUtils::writeDelimitedTo(
        cmd_resp, &output_stream);
    if (!resp_write_success) {
      QUIC_LOG(WARNING) << "failed to serialize response";
      CloseClient(client);
      return false;
    }
    client.response_len += output_stream.ByteCount();
    client.response_buf.resize(client.response_len);
    epoll_server_->StartWrite(client.fd);
  }
  if (consumed > filled_len) {
    QUIC_LOG(ERROR) << "internal error: consumed(" << consumed
                    << ") more bytes than were available(" << filled_len
                    << ")";
    CloseClient(client);
    return false;
  }
  if (consumed == filled_len) {
    QUIC_LOG(WARNING) << "full read of " << consumed << " consumed";
    filled_len = 0;
  } else {
    QUIC_LOG(WARNING) << "partial read of " << consumed << " consumed with "
                      << (filled_len - consumed) << " left";
    memmove(&client_buffer[0], &client_buffer[consumed], filled_len - consumed);
    filled_len -= consumed;
  }
  size_t minbufsize = std::max((size_t)kDefaultReadBufsize, filled_len);
  if (client_buffer.size() > minbufsize) {
    client_buffer.resize(minbufsize);
  }

  if (filled_len > 0 && just_ate > 0 && err == kQPP_NoError) {
    // we have more data and we might have more commands to parse.
    // yield to avoid blocking other things but schedule another entry
    // immediately.
    return true;
  }
  return false;
}

bool QuicEpollUDSPusher::CloseClient(QuicEpollUDSPusher::Client& client) {
  QUIC_LOG(WARNING) << "closing client(" << client.fd << ")";
  epoll_server_->UnregisterFD(client.fd);
  close(client.fd);
  clients_.erase(client.fd);
  delete(&client);
  return true;
}

// From EpollCallbackInterface
void QuicEpollUDSPusher::OnRegistration(QuicEpollServer* eps,
                                        int fd,
                                        int event_mask) {
  QUIC_LOG(WARNING) << "OnRegistration(" << path_ << ")";
}
void QuicEpollUDSPusher::OnModification(int fd, int event_mask) {
  QUIC_LOG(WARNING) << "OnModification(" << path_ << ")";
}

void QuicEpollUDSPusher::OnEvent(int fd, QuicEpollEvent* event) {

  QUIC_LOG(WARNING) << "OnEvent(" << path_ << ")";
  event->out_ready_mask = 0;
  event_count_ += 1;
  Client* client = 0;

  if (event->in_events & EPOLLIN) {
    if (fd == topcmd_fd_) {
      // Connection from the UDS
      handleConnection();
      return;
    } else {
      // a particular client
      auto it = clients_.find(fd);
      
      if (it == clients_.end()) {
        QUIC_LOG(ERROR) << "Client isn't registered";
        Shutdown();
        return;
      }
      client = it->second;
      handleComingData(*client);
    }
    bool refire = RunBufferedCommands(*client);
    if (refire) {
      event->out_ready_mask |= EPOLLIN;
    }
  }

  if (event->in_events & EPOLLOUT) {
    if (!client) {
      // a particular client
      auto it = clients_.find(fd);
      
      if (it == clients_.end()) {
        QUIC_LOG(ERROR) << "Client isn't registered (on EPOLLOUT)";
        Shutdown();
        return;
      }
      client = it->second;
    }
    // QUIC_DVLOG(1)
    QUIC_LOG(WARNING) << "EPOLLOUT(client(" << fd << "))";
    if (client->response_len > 0) {
      size_t wrote = write(client->fd, &client->response_buf[0],
          client->response_len);
      if (wrote < 0) {
        int got_err = errno;
        QUIC_LOG(ERROR) << "error(" << got_err << ") writing to client(" << fd << "): " << get_strerror(got_err);
        CloseClient(*client);
        return;
      }
      if (wrote > client->response_len) {
        QUIC_LOG(ERROR) << "error writing to client(" << fd << "): wrote " << wrote << " on write of " << client->response_len << " bytes";
        CloseClient(*client);
        return;
      }
      if (wrote < client->response_len) {
        client->response_len -= wrote;
        client->response_buf.erase(client->response_buf.begin(),
            client->response_buf.begin() + wrote);
        event->out_ready_mask |= EPOLLOUT;
      } else {
        client->response_len = 0;
        epoll_server_->StopWrite(client->fd);
      }
    }
  }

  if (event->in_events & EPOLLHUP) {
    // QUIC_DVLOG(1)
    QUIC_LOG(WARNING) << "EPOLLHUP(client(" << fd << "))";
    CloseClient(*client);
    return;
  }

  if (event->in_events & EPOLLERR) {
    // QUIC_DVLOG(1)
    QUIC_LOG(WARNING) << "EPOLLERR(client(" << fd << "))";
    CloseClient(*client);
    return;
  }
  /*
  if (event_count_ % 10 == 0) {
    QUIC_LOG(WARNING)
      << "Shutdown/Reinitialize at event count=" << event_count_;
    Shutdown();
    Initialize();
    return;
  }
  */
}

void QuicEpollUDSPusher::OnUnregistration(int fd, bool replaced) {
  QUIC_LOG(WARNING) << "OnUnregistration(" << path_ << ")";
}
void QuicEpollUDSPusher::OnShutdown(QuicEpollServer* eps, int fd) {
  QUIC_LOG(WARNING) << "OnShutdown(" << path_ << ")";
  Shutdown();
}

void QuicEpollUDSPusher::handleConnection() {
  struct sockaddr_un remote;
  unsigned int sock_len = 0;

  Client *current_client = new Client();
  Client& client = *current_client;

  QUIC_LOG(INFO) << ("Waiting for connection.... \n");

  if ((client.fd = accept(topcmd_fd_, (struct sockaddr*)&remote, &sock_len)) == -1) {
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err << ") accepting on \"" << path_
                    << "\": " << get_strerror(got_err);
    delete current_client;
    return;
  }

  QUIC_LOG(INFO) << ("Server connected \n");

  epoll_server_->RegisterFDForRead(client.fd, this);

  clients_.insert(std::make_pair(client.fd, current_client));
}

void QuicEpollUDSPusher::handleComingData(Client& client) {
  
  auto& fd = client.fd;
  auto& filled_len = client.filled_len;
  auto& client_buffer = client.buffer;

  QUIC_LOG(WARNING) << "EPOLLIN(" << path_ << ")";
  enum {
    kMinReadSpace = 1024 * 4,
    kLoopLimit = 5,
    kBufStopLimit = 1024 * 1024,
  };
  int loopcount = 0;

  size_t minbufsize = std::max((size_t)kDefaultReadBufsize,
                               (size_t)(filled_len + kMinReadSpace));
  if (client_buffer.size() < minbufsize) {
    client_buffer.resize(minbufsize);
  }
  ssize_t len;
  while (loopcount < kLoopLimit && filled_len <= kBufStopLimit) {
    len = read(fd, &client_buffer[filled_len], client_buffer.size() - filled_len);
    if (len < 0) {
      int got_err = errno;
      if (got_err == EAGAIN || got_err == EWOULDBLOCK) {
        len = 0;
        break;
      }
      QUIC_LOG(ERROR) << "error(" << got_err << ") reading from fifo \""
                      << path_ << "\": " << get_strerror(got_err)
                      << ", shutting down";
      CloseClient(client);
      return;
    }
    if (len == 0) {
      break;
    }
    filled_len += len;
    loopcount += 1;
  }
}

bool QuicEpollUDSPusher::parseCommand(
    QuicEpollUDSPusher::Client& client,
    quic::PushCmd& input_cmd,
    size_t size,
    const uint8_t* buf,
    ssize_t& consumed,
    QuicPushParseErrorCode& err) {

  google::protobuf::io::ArrayInputStream input_stream(buf, size);

  bool read_success =
      quic::PushUtils::readDelimitedFrom(&input_stream, &input_cmd, &consumed);

  return read_success;
}

void QuicEpollUDSPusher::RunCommand(const quic::PushCmd& cmd,
    quic::PushCmd_Response& resp)
{
  switch (cmd.Command_case()) {
    case quic::PushCmd::kMakePool: {
      QUIC_LOG(WARNING) << "makepool id:" << cmd.make_pool().pool_id();
      resp.set_success(true);
      resp.mutable_make_pool_resp()->set_pool_token("hi2");
      break;
    }
    case quic::PushCmd::kMakeChannel: {
      QUIC_LOG(WARNING) << "makechannel pool_token:" << cmd.make_channel().pool_token();
      resp.set_success(true);
      resp.mutable_make_channel_resp()->set_channel_token("hi3");
      break;
    }
    case quic::PushCmd::kFullStream: {
      QUIC_LOG(WARNING) << "fullstream data size:" << cmd.full_stream().data().size();
      resp.set_success(true);
      break;
    }
    default: {
      resp.set_success(false);
      std::ostringstream str;
      str << "unimplemented command: " << (int)cmd.Command_case();
      resp.set_error_message(str.str());
    }
  }
}

}  // namespace quic
