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
#include <algorithm>
#include <utility>

#include "quic/platform/api/quic_logging.h"
#include "quic/tools/quic_server.h"

namespace {

const char kTopPath[] = "/top-cmd";

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

QuicEpollUDSPusher::QuicEpollUDSPusher(std::string path_base,
                                       QuicEpollServer* epoll_server,
                                       QuicSpdyServerBase* server)
    : path_base_(path_base),
      socket_path_("/tmp/test.sock"),
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
  ;

  QUIC_LOG(INFO) << "Initialize(" << path_base_ << ")";

  top_path_ = path_base_ + kTopPath;

  int s = 0;
  struct sockaddr_un local;
  int len = 0;

  s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (-1 == s) {
    QUIC_LOG(WARNING) << "Error on socket() call \n";
    return 1;
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, socket_path_.c_str());
  unlink(local.sun_path);

  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(s, (struct sockaddr*)&local, len) != 0) {
    QUIC_LOG(WARNING) << "Error on binding socket \n";
    return 1;
  }

  if (listen(s, max_incoming_connections_) != 0) {
    QUIC_LOG(WARNING) << "Error on listen call \n";
  }

  topcmd_fd_ = s;
  epoll_server_->RegisterFDForRead(topcmd_fd_, this);

  return true;
}
void QuicEpollUDSPusher::Shutdown() {
  QUIC_LOG(WARNING) << "Shutdown(" << top_path_ << ")";
  if (topcmd_fd_ != -1) {
    epoll_server_->UnregisterFD(topcmd_fd_);
    int rc;
    rc = close(topcmd_fd_);
    if (rc != 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err << ") closing \"" << top_path_
                      << "\": " << get_strerror(got_err);
    }
    topcmd_fd_ = -1;
    rc = unlink(top_path_.c_str());
    if (rc != 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err << ") unlinking \"" << top_path_
                      << "\": " << get_strerror(got_err);
    }
  }
}

bool QuicEpollUDSPusher::RunBufferedCommands() {

  Client& client = *current_client;
  auto& filled_len = client.filled_len;
  auto& client_buffer = client.buffer;

  enum { kCommandLimit = 10 };
  int commands = 0;
  // TODO: maybe a time limit is better than a command count limit?  Or
  //       do we need both?  --jake 2022-04-24

  quic::PushServer top_cmd;
  top_cmd.mutable_activate_pool()->set_pool_id(42);

  size_t consumed = 0;
  ssize_t just_ate = 0;
  QuicPushParseErrorCode err = kQPP_NoError;
  QuicPushCommandBase::CommandArgs args;
  args.server = (QuicServer*)server_;
  while (consumed < filled_len && err == kQPP_NoError &&
         commands < kCommandLimit) {
    just_ate = 0;

    quic::PushServer push_cmd;

    auto read_success = parseCommand(push_cmd, client_buffer.size() - consumed,
                                     &client_buffer[consumed], just_ate, err);
    if (!read_success) {
      QUIC_LOG(WARNING) << "no command parsed from " << filled_len
                        << " bbytes at offset " << consumed << " (" << just_ate
                        << " output as consumed)";
      break;
    }

    QUIC_LOG(WARNING) << "received command" << push_cmd.DebugString();

    QUIC_LOG(WARNING) << "command parsed from " << filled_len
                      << " bytes at offset " << consumed << " consuming "
                      << just_ate;
    // assert(just_ate > 0);
    consumed += just_ate;
    commands += 1;

    RunCommand(push_cmd);
  }
  if (consumed > filled_len) {
    QUIC_LOG(ERROR) << "internal error: consumed(" << consumed
                    << ") more bytes than were available(" << filled_len
                    << ")";
    Shutdown();
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

// From EpollCallbackInterface
void QuicEpollUDSPusher::OnRegistration(QuicEpollServer* eps,
                                        int fd,
                                        int event_mask) {
  QUIC_LOG(WARNING) << "OnRegistration(" << top_path_ << ")";
}
void QuicEpollUDSPusher::OnModification(int fd, int event_mask) {
  QUIC_LOG(WARNING) << "OnModification(" << top_path_ << ")";
}

void QuicEpollUDSPusher::OnEvent(int fd, QuicEpollEvent* event) {

  QUIC_LOG(WARNING) << "OnEvent(" << top_path_ << ")";
  event->out_ready_mask = 0;
  event_count_ += 1;

  if (event->in_events & EPOLLIN) {
    if (fd == topcmd_fd_)  // Connection from the UDS
    {
      handleConnection();
      return ;
    }
    else
    {
      auto it = clients_.find(fd);
      
      if (it != clients_.end())
      {
        current_client = it->second;
        handleComingData();
      }
      else
        QUIC_LOG(ERROR) << "Client isn't registered";
    }
  }


  bool refire = RunBufferedCommands();
  if (refire) {
    event->out_ready_mask |= EPOLLIN;
  }

  if (event->in_events & EPOLLHUP) {
    // QUIC_DVLOG(1)
    QUIC_LOG(WARNING) << "EPOLLHUP(" << top_path_ << ")";
    epoll_server_->UnregisterFD(fd);
    close(fd);
    return;
  }

  if (event->in_events & EPOLLOUT) {
    // QUIC_DVLOG(1)
    QUIC_LOG(WARNING) << "EPOLLOUT(" << top_path_ << ")";
    QUIC_LOG(ERROR) << "unexpected writable event for read-only pipe "
                    << top_path_ << ", shutting down";
    epoll_server_->UnregisterFD(fd);
    close(fd);
    return;
  }

  if (event->in_events & EPOLLERR) {
    // QUIC_DVLOG(1)
    QUIC_LOG(WARNING) << "EPOLLERR(" << top_path_ << ")";
    epoll_server_->UnregisterFD(fd);
    close(fd);
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
  QUIC_LOG(WARNING) << "OnUnregistration(" << top_path_ << ")";
}
void QuicEpollUDSPusher::OnShutdown(QuicEpollServer* eps, int fd) {
  QUIC_LOG(WARNING) << "OnShutdown(" << top_path_ << ")";
  Shutdown();
}

void QuicEpollUDSPusher::handleConnection() {
  struct sockaddr_un remote;
  unsigned int sock_len = 0;

  Client& client = *new Client;

  QUIC_LOG(INFO) << ("Waiting for connection.... \n");

  if ((client.fd = accept(topcmd_fd_, (struct sockaddr*)&remote, &sock_len)) == -1) {
    QUIC_LOG(ERROR) << ("Error on accept() call \n");
    return;
  }

  QUIC_LOG(INFO) << ("Server connected \n");

  epoll_server_->RegisterFDForRead(client.fd, this);

  clients_.insert(std::make_pair(client.fd, &client));
}

void QuicEpollUDSPusher::handleComingData() {
  
  Client& client = *current_client;
  auto& fd = client.fd;
  auto& filled_len = client.filled_len;
  auto& client_buffer = client.buffer;

  QUIC_LOG(WARNING) << "EPOLLIN(" << top_path_ << ")";
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
                      << top_path_ << "\": " << get_strerror(got_err)
                      << ", shutting down";
      Shutdown();
      return;
    }
    if (len == 0) {
      break;
    }
    filled_len += len;
    loopcount += 1;
  }
}

bool QuicEpollUDSPusher::parseCommand(quic::PushServer& input_cmd,
                                      size_t size,
                                      const uint8_t* buf,
                                      ssize_t& consumed,
                                      QuicPushParseErrorCode& err) {

  auto &client_buffer = current_client->buffer;
                                        
  google::protobuf::io::ArrayInputStream input_stream(client_buffer.data(),
                                                      client_buffer.size());

  bool read_success =
      quic::PushUtils::readDelimitedFrom(&input_stream, &input_cmd, &consumed);

  return read_success;
}

void QuicEpollUDSPusher::RunCommand(const quic::PushServer& cmd)
{
  /*if (cmd.Command_base())
  {
    
  }
  else*/
   QUIC_LOG(WARNING) << "pool id from the message:" << cmd.activate_pool().pool_id();
}

}  // namespace quic
