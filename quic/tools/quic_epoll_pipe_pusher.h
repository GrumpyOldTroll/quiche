// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_EPOLL_PIPE_PUSHER_H_
#define QUICHE_QUIC_TOOLS_EPOLL_PIPE_PUSHER_H_

#include <string>
#include <vector>
#include <memory>

#include "quic/platform/api/quic_epoll.h"
#include "quic/tools/quic_pusher.h"
#include "quic/tools/quic_push_commands.h"
#include "quic/tools/quic_spdy_server_base.h"

namespace quic {

// Factory creating QuicPusher instances.
class QuicEpollPipePusher : public QuicPusher,
                            public QuicEpollCallbackInterface {
 public:
  QuicEpollPipePusher(
      std::string path_base,
      QuicSpdyServerBase* server);

  ~QuicEpollPipePusher() override;

  bool Initialize();
  void Shutdown();
  bool RunBufferedCommands();

  // From EpollCallbackInterface
  void OnRegistration(QuicEpollServer* eps, int fd,
                      int event_mask) override;
  void OnModification(int fd, int event_mask) override;
  void OnEvent(int fd, QuicEpollEvent* event) override;
  void OnUnregistration(int fd, bool replaced) override;
  void OnShutdown(QuicEpollServer* eps, int fd) override;
  std::string Name() const override { return "QuicEpollPipePusher"; }

 private:
  std::string path_base_;
  std::string top_path_;
  std::vector<std::unique_ptr<QuicPushCommandBase> > commands_;
  std::vector<uint8_t> parse_buf_;
  size_t filled_len_;
  int topcmd_fd_;
  QuicSpdyServerBase* server_;  // unowned.
  QuicEpollServer* epoll_server_;  // unowned.
  unsigned int event_count_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_EPOLL_PIPE_PUSHER_H_
