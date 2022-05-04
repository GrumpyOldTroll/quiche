// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_epoll_pipe_pusher.h"

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <features.h>
#include <algorithm>
#include <utility>

#include "quic/platform/api/quic_logging.h"
#include "quic/tools/quic_server.h"
#include "quic/core/proto/push_server_proto.h"

namespace {

const char kTopPath[] = "/top-cmd";

enum {
  kDefaultReadBufsize = 1024*16,
};

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

/*
std::size_t stringstream_buffersize(const std::stringstream& ss) {
  std::streambuf* buf = ss.rdbuf();
  std::streampos pos = buf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  std::streampos end = buf->pubseekoff(0, std::ios_base::end, std::ios_base::in);
  buf->pubseekpos(pos, std::ios_base::in);
  return end - pos;
}
*/

}  // namespace

namespace quic {

QuicEpollPipePusher::QuicEpollPipePusher(
    std::string path_base,
    QuicEpollServer* epoll_server,
    QuicSpdyServerBase* server)
    : path_base_(path_base),
      filled_len_(0),
      topcmd_fd_(-1),
      epoll_server_(epoll_server),
      server_(server),
      event_count_(0) {}

QuicEpollPipePusher::~QuicEpollPipePusher() {
  Shutdown();
}

bool QuicEpollPipePusher::Initialize() {
  QUIC_LOG(INFO) << "Initialize(" << path_base_ << ")";

  mode_t old_mask = umask(S_IRWXG|S_IRWXO);
  mode_t check_mask = umask(S_IRWXG|S_IRWXO);

  QUIC_LOG(WARNING) << "so far so good(warn), old mask=0" << std::oct << old_mask <<", new=0"<<(check_mask)<< ", O_RDWR=" << O_RDWR << std::dec;

  top_path_ = path_base_ + kTopPath;
  int rc;
  rc = mkfifo(top_path_.c_str(), S_IRUSR|S_IWUSR);
  if (rc < 0) {
    umask(old_mask);
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") creating fifo \"" << top_path_ << "\": " << get_strerror(got_err);
    return false;
  }

  // NB: if you use O_RDONLY you get infinite POLLHUP events after the first write.
  rc = open(top_path_.c_str(), O_RDWR);
  if (rc < 0) {
    umask(old_mask);
    int got_err = errno;
    QUIC_LOG(ERROR) << "error(" << got_err <<") opening new fifo \"" << top_path_ << "\": " << get_strerror(got_err);
    rc = 0;
    rc = unlink(top_path_.c_str());
    if (rc < 0) {
      got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err <<") unlinking new fifo \"" << top_path_ << "\": " << get_strerror(got_err);
    }
    return false;
  }
  umask(old_mask);
  topcmd_fd_ = rc;
  epoll_server_->RegisterFDForRead(topcmd_fd_, this);

  return true;
}
void QuicEpollPipePusher::Shutdown() {
  QUIC_LOG(WARNING) << "Shutdown(" << top_path_ << ")";
  if (topcmd_fd_ != -1) {
    epoll_server_->UnregisterFD(topcmd_fd_);
    int rc;
    rc = close(topcmd_fd_);
    if (rc != 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err <<") closing \"" << top_path_ << "\": " << get_strerror(got_err);
    }
    topcmd_fd_ = -1;
    rc = unlink(top_path_.c_str());
    if (rc != 0) {
      int got_err = errno;
      QUIC_LOG(ERROR) << "error(" << got_err <<") unlinking \"" << top_path_ << "\": " << get_strerror(got_err);
    }
    filled_len_ = 0;
  }
}

bool QuicEpollPipePusher::RunBufferedCommands() {
  enum {
    kCommandLimit = 10
  };
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
  while (consumed < filled_len_ && err == kQPP_NoError && commands < kCommandLimit) {
    just_ate = 0;
    auto push_cmd = ParsePushCommand(parse_buf_.size() - consumed,
        &parse_buf_[consumed], just_ate, err);
    if (!push_cmd) {
      QUIC_LOG(WARNING) << "no command parsed from " << filled_len_ << " bbytes at offset " << consumed << " (" << just_ate << " output as consumed)";
      break;
    }
    QUIC_LOG(WARNING) << "command parsed from " << filled_len_ << " bytes at offset " << consumed << " consuming " << just_ate;
    // assert(just_ate > 0);
    consumed += just_ate;
    commands += 1;
    push_cmd->RunCommand(args);
  }
  if (consumed > filled_len_) {
    QUIC_LOG(ERROR) << "internal error: consumed(" << consumed << ") more bytes than were available(" << filled_len_ << ")";
    Shutdown();
    return false;
  }
  if (consumed == filled_len_) {
    QUIC_LOG(WARNING) << "full read of " << consumed << " consumed";
    filled_len_ = 0;
  } else {
    QUIC_LOG(WARNING) << "partial read of " << consumed << " consumed with " << (filled_len_-consumed) << " left";
    memmove(&parse_buf_[0], &parse_buf_[consumed], filled_len_-consumed);
    filled_len_ -= consumed;
  }
  size_t minbufsize = std::max((size_t)kDefaultReadBufsize, filled_len_);
  if (parse_buf_.size() > minbufsize) {
    parse_buf_.resize(minbufsize);
  }

  if (filled_len_ > 0 && just_ate > 0 && err == kQPP_NoError) {
    // we have more data and we might have more commands to parse.
    // yield to avoid blocking other things but schedule another entry
    // immediately.
    return true;
  }
  return false;
}

// From EpollCallbackInterface
void QuicEpollPipePusher::OnRegistration(
    QuicEpollServer* eps,
    int fd,
    int event_mask) {
  QUIC_LOG(WARNING) << "OnRegistration(" << top_path_ << ")";
  QUICHE_DCHECK_EQ(fd, topcmd_fd_);
}
void QuicEpollPipePusher::OnModification(int fd, int event_mask) {
  QUIC_LOG(WARNING) << "OnModification(" << top_path_ << ")";
  QUICHE_DCHECK_EQ(fd, topcmd_fd_);
}
void QuicEpollPipePusher::OnEvent(int fd, QuicEpollEvent* event) {
  QUICHE_DCHECK_EQ(fd, topcmd_fd_);

  QUIC_LOG(WARNING) << "OnEvent(" << top_path_ << ")";
  event->out_ready_mask = 0;
  event_count_ += 1;

  if (event->in_events & EPOLLIN) {
    //QUIC_DVLOG(1) 
    QUIC_LOG(WARNING)
      << "EPOLLIN(" << top_path_ << ")";
    enum {
      kMinReadSpace = 1024*4,
      kLoopLimit = 5,
      kBufStopLimit = 1024*1024,
    };
    int loopcount = 0;

    size_t minbufsize = std::max((size_t)kDefaultReadBufsize, (size_t)(filled_len_ + kMinReadSpace));
    if (parse_buf_.size() < minbufsize) {
      parse_buf_.resize(minbufsize);
    }
    ssize_t len;
    while (loopcount < kLoopLimit && filled_len_ <= kBufStopLimit) {
      len = read(fd, &parse_buf_[filled_len_], parse_buf_.size() - filled_len_);
      if (len < 0) {
        int got_err = errno;
        if (got_err == EAGAIN || got_err == EWOULDBLOCK) {
          len = 0;
          break;
        }
        QUIC_LOG(ERROR) << "error(" << got_err <<") reading from fifo \"" << top_path_ << "\": " << get_strerror(got_err) << ", shutting down";
        Shutdown();
        return;
      }
      if (len == 0) {
        break;
      }
      filled_len_ += len;
      loopcount += 1;
    }
  }
  bool refire = RunBufferedCommands();
  if (refire) {
    event->out_ready_mask |= EPOLLIN;
  }

  if (event->in_events & EPOLLHUP) {
    // QUIC_DVLOG(1) 
    QUIC_LOG(WARNING)
      << "EPOLLHUP(" << top_path_ << ")";
    Shutdown();
    return;
  }
  if (event->in_events & EPOLLOUT) {
    // QUIC_DVLOG(1) 
    QUIC_LOG(WARNING)
      << "EPOLLOUT(" << top_path_ << ")";
    QUIC_LOG(ERROR) << "unexpected writable event for read-only pipe " << top_path_ << ", shutting down";
    Shutdown();
    return;
  }
  if (event->in_events & EPOLLERR) {
    // QUIC_DVLOG(1) 
    QUIC_LOG(WARNING)
      << "EPOLLERR(" << top_path_ << ")";
    Shutdown();
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
void QuicEpollPipePusher::OnUnregistration(int fd, bool replaced) {
  QUIC_LOG(WARNING) << "OnUnregistration(" << top_path_ << ")";
  QUICHE_DCHECK_EQ(fd, topcmd_fd_);
}
void QuicEpollPipePusher::OnShutdown(QuicEpollServer* eps, int fd) {
  QUIC_LOG(WARNING) << "OnShutdown(" << top_path_ << ")";
  QUICHE_DCHECK_EQ(fd, topcmd_fd_);
  Shutdown();
}

}  // namespace quic
