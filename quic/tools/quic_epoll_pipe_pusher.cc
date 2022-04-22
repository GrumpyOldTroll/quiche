// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/tools/quic_epoll_pipe_pusher.h"

#include <utility>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <features.h>

#include "quic/platform/api/quic_logging.h"
#include "quic/tools/quic_server.h"

namespace {

const char kTopPath[] = "/top-cmd";

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

#if TEMP_QPUSH_BEFORE_COMMANDS
std::size_t stringstream_buffersize(const std::stringstream& ss) {
  std::streambuf* buf = ss.rdbuf();
  std::streampos pos = buf->pubseekoff(0, std::ios_base::cur, std::ios_base::in);
  std::streampos end = buf->pubseekoff(0, std::ios_base::end, std::ios_base::in);
  buf->pubseekpos(pos, std::ios_base::in);
  return end - pos;
}
#endif

}  // namespace

namespace quic {

QuicEpollPipePusher::QuicEpollPipePusher(
    std::string path_base,
    QuicSpdyServerBase* server)
    : path_base_(path_base),
      filled_len_(0),
      topcmd_fd_(-1),
      server_(server),
      epoll_server_(0),
      event_count_(0) {}

QuicEpollPipePusher::~QuicEpollPipePusher() {
  Shutdown();
}

bool QuicEpollPipePusher::Initialize() {
  QUIC_LOG(INFO) << "Initialize(" << path_base_ << ")";

  // TODO: can't use dynamic cast, this is not the right thing to do.
  // (error: use of dynamic_cast requires -frtti)
  // The right thing is probably for epoll_server to be passed into both
  // QuicServer and this object, rather than being owned by QuicServer.
  // An easier but worse hack would be to add a null epoll_server virtual
  // function to QuicSpdyServerBase to expose the override to pull it out.
  // In practice today QuicEpollPipePusher is used when and only when the
  // server is actually a QuicServer, so this is fragile but safe for the
  // moment (I think?  At least when building epoll_quic_server...)
  // --jake 2022-04
  // QuicServer* server = dynamic_cast<QuicServer*>(server_);
  QuicServer* server = static_cast<QuicServer*>(server_);
  if (!server) {
    QUIC_LOG(ERROR) << "Cannot extract EpollServer from server";
    return false;
  }

  epoll_server_ = server->epoll_server();
  if (!epoll_server_) {
    QUIC_LOG(ERROR) << "Null EpollServer from QuicServer";
    return false;
  }

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
    topcmd_buf_.clear();
  }
}

bool QuicEpollPipePusher::RunBufferedCommands() {
  // TODO: read full commands from the stream and execute them (if
  // partial command, put it back in the stream).
  enum {
    kCommandLimit = 5
  };
  int commands = 0;
  std::string cmd;
  while (topcmd_buf_ >> cmd && commands < kCommandLimit) {
    QUIC_LOG(WARNING) << "Read from cmd buf: \"" << cmd << "\"";
    commands += 1;
  }
  if (!topcmd_buf_ || topcmd_buf_.eof()) {
    topcmd_buf_.clear();
  }

  if (commands >= kCommandLimit) {
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

#if TEMP_QPUSH_BEFORE_COMMANDS
  // currently disabled
#if 1
  if (event->in_events & EPOLLIN) {
    //QUIC_DVLOG(1) 
    QUIC_LOG(WARNING)
      << "EPOLLIN(" << top_path_ << ")";
    enum {
      kBufsize = 1024*16,
      kLoopLimit = 5,
      kBufStopLimit = 1024*1024
    };
    char buf[kBufsize];
    ssize_t len;
    int loopcount = 0;
    ssize_t cur_size = stringstream_buffersize(topcmd_buf_);

    while (loopcount < kLoopLimit && cur_size <= kBufStopLimit) {
      loopcount += 1;
      len = read(fd, buf, sizeof(buf));
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
      topcmd_buf_.write(buf, len);
      cur_size += len;
    }
    if (cur_size > kBufStopLimit) {
      QUIC_LOG(ERROR) << "exceeded max command buffer size for " << top_path_ << ", shutting down";
      Shutdown();
      return;
    }
    ssize_t post_size = stringstream_buffersize(topcmd_buf_);
    if (cur_size != post_size) {
      QUIC_LOG(ERROR) << "cur_size(" << cur_size <<") != post_size(" << post_size <<"); bad stringstream_buffersize";
    } else {
      QUIC_LOG(WARNING) << "cur_size(" << cur_size <<") == post_size(" << post_size <<"); good stringstream_buffersize";
    }

    // set this if for some reason we are yielding but want another
    // callback right away without waiting for an actual socket write:
    if (loopcount == kLoopLimit && len != 0) {
      event->out_ready_mask |= EPOLLIN;
    }
  }
  bool refire = RunBufferedCommands();
  ssize_t remaining_size = stringstream_buffersize(topcmd_buf_);
  if (remaining_size > 0) {
    QUIC_LOG(WARNING) << "remaining_size(" << remaining_size <<") after command processing";
  }
#endif  // 0
#else  // TEMP_QPUSH_BEFORE_COMMANDS
  bool refire = false;
  if (event->in_events & EPOLLIN) {
    //QUIC_DVLOG(1) 
    QUIC_LOG(WARNING)
      << "EPOLLIN(" << top_path_ << ")";
    enum {
      kBufsize = 1024*16,
      kLoopLimit = 5,
      kBufStopLimit = 1024*1024,
      kCommandLimit = 10,
    };
    int loopcount = 0;
    if (parse_buf_.size() < filled_len_ + kBufsize) {
      parse_buf_.resize(filled_len_ + kBufsize);
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
      filled_len_ += len;
      if ((size_t)len < parse_buf_.size()) {
        break;
      }
      loopcount += 1;
    }

    ssize_t consumed = 0;
    ssize_t just_ate = 0;
    QuicPushParseErrorCode err = kQPP_NoError;
    QuicPushCommandBase::CommandArgs args;
    args.server = (QuicServer*)server_;
    while (err == kQPP_NoError) {
      just_ate = 0;
      auto push_cmd = ParsePushCommand(parse_buf_.size() - consumed,
          &parse_buf_[consumed], just_ate, err);
      if (!push_cmd) {
        if (just_ate < 0) {
          // TODO: move current data to front, make sure there's more than -just_ate space, and read more if we've got any
          break;
        }
        break;
      }
      // assert(just_ate > 0);
      consumed += just_ate;
      push_cmd->RunCommand(args);
    }
    parse_buf_.clear();  // just because I'm tired but want to try it once --jake 2022-04-22 3:52am
  }
#endif  // TEMP_QPUSH_BEFORE_COMMANDS
  (void)filled_len_;
  (void)parse_buf_;
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
