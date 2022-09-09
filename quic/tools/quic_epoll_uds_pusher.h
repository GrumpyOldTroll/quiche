// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_EPOLL_PIPE_PUSHER_H_
#define QUICHE_QUIC_TOOLS_EPOLL_PIPE_PUSHER_H_

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "quic/platform/api/quic_epoll.h"
#include "quic/tools/quic_pusher.h"
#include "quic/tools/quic_pusher_pool.h"
#include "quic/tools/quic_push_commands.h"
#include "quic/tools/quic_push_utils.h"
#include "quic/tools/quic_epoll_uds_client.h"
#include "quic/tools/quic_spdy_server_base.h"
#include "quic/tools/quic_simple_server_backend.h"
#include "quic/core/proto/push_server_proto.h"

namespace quic {

class QuicServer;

// Factory creating QuicPusher instances.
class QuicEpollUDSPusher : public QuicPusher,
                            public QuicEpollCallbackInterface {
 public:
  typedef QuicEpollUDSClient::FDType FDType;
  typedef QuicEpollUDSClient Client;

  QuicEpollUDSPusher(
      std::string path_base,
      QuicEpollServer* epoll_server,
      QuicSpdyServerBase* server,
      QuicSimpleServerBackend* backend);

  ~QuicEpollUDSPusher() override;

  bool Initialize();
  void Shutdown();
  bool RunBufferedCommands(Client& client);

  bool CloseClient(Client& client);

  // From EpollCallbackInterface
  void OnRegistration(QuicEpollServer* eps, FDType fd,
                      int event_mask) override;
  void OnModification(FDType fd, int event_mask) override;
  void OnEvent(FDType fd, QuicEpollEvent* event) override;
  void OnUnregistration(FDType fd, bool replaced) override;
  void OnShutdown(QuicEpollServer* eps, FDType fd) override;

  void handleConnection();
  void handleComingData(Client& client);
  std::string Name() const override { return "QuicEpollUDSPusher"; }

  bool parseCommand(
    Client& client,
    quic::PushCmd& input_cmd,
    size_t size,
    const uint8_t* buf,
    ssize_t &consumed,
    QuicPushParseErrorCode &err) ;

  void RunCommand(const quic::PushCmd& cmd, quic::PushCmd_Response& response);

 private:
  std::string path_;
  
  std::vector<std::unique_ptr<QuicPushCommandBase> > commands_;
  std::unordered_map<FDType, Client*> clients_; 
  std::unordered_map<std::string, std::shared_ptr<QuicPusherPool> > pushers_;

  int topcmd_fd_;
  QuicEpollServer* epoll_server_;  // unowned.
  QuicSpdyServerBase* server_;  // unowned.
  QuicSimpleServerBackend* backend_;  // unowned.
  unsigned int event_count_;
  int max_incoming_connections_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_EPOLL_PIPE_PUSHER_H_
