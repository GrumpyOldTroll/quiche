// Copyright (c) 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_TOOLS_WEB_TRANSPORT_PUSH_VISITORS_H_
#define QUICHE_QUIC_TOOLS_WEB_TRANSPORT_PUSH_VISITORS_H_

#include <string>
#include <memory>

#include "quic/core/web_transport_interface.h"
#include "quic/tools/quic_pusher_pool.h"
#include "quic/platform/api/quic_logging.h"
#include "common/platform/api/quiche_mem_slice.h"
#include "common/quiche_circular_deque.h"
#include "common/simple_buffer_allocator.h"

namespace quic {

// Sends supplied data.
class WebTransportUnidirectionalPushWriteVisitor
    : public WebTransportStreamVisitor {
 public:
  WebTransportUnidirectionalPushWriteVisitor(WebTransportStream* stream,
                                             const std::string& data)
      : stream_(stream), data_(data) {}

  void OnCanRead() override { QUIC_NOTREACHED(); }
  void OnCanWrite() override {
    if (data_.empty()) {
      return;
    }
    if (!stream_->Write(data_)) {
      return;
    }
    data_ = "";
    bool fin_sent = stream_->SendFin();
    QUICHE_DVLOG(1)
        << "WebTransportUnidirectionalPushWriteVisitor finished sending data.";
    QUICHE_DCHECK(fin_sent);
  }

  void OnResetStreamReceived(WebTransportStreamError /*error*/) override {
    QUIC_LOG(WARNING) << "ignoring Reset Stream received from WebTransportUnidirectionalPushWriteVisitor";
  }
  void OnStopSendingReceived(WebTransportStreamError /*error*/) override {
    QUIC_LOG(WARNING) << "ignoring Stop Sending received from WebTransportUnidirectionalPushWriteVisitor";
  }
  void OnWriteSideInDataRecvdState() override {
    QUIC_LOG(WARNING) << "ignoring WriteSideInDataRecvdState from WebTransportUnidirectionalPushWriteVisitor";
  }

 private:
  WebTransportStream* stream_;
  std::string data_;
};

// A session visitor which sets unidirectional or bidirectional stream visitors
// to echo.
class PushWebTransportSessionVisitor : public WebTransportVisitor {
 public:
  PushWebTransportSessionVisitor(
      QuicPusherPool* pusher_pool,
      WebTransportSession* session)
      : pusher_pool_(pusher_pool), session_(session) {}

  void OnSessionReady(const spdy::SpdyHeaderBlock&) override {
    pusher_pool_->VisitorReady(this);
    if (session_->CanOpenNextOutgoingBidirectionalStream()) {
      OnCanCreateNewOutgoingBidirectionalStream();
    }
    if (session_->CanOpenNextOutgoingUnidirectionalStream()) {
      OnCanCreateNewOutgoingUnidirectionalStream();
    }
  }

  void OnSessionClosed(WebTransportSessionError /*error_code*/,
                       const std::string& /*error_message*/) override {
    pusher_pool_->VisitorStopped(this);
  }

  void OnIncomingBidirectionalStreamAvailable() override {
    /*
    ignore.  we're push-unidirectional-only

    while (true) {
      WebTransportStream* stream =
          session_->AcceptIncomingBidirectionalStream();
      if (stream == nullptr) {
        return;
      }
      QUIC_DVLOG(1)
          << "PushWebTransportSessionVisitor received a bidirectional stream "
          << stream->GetStreamId();
      stream->SetVisitor(
          std::make_unique<WebTransportBidirectionalPushVisitor>(stream));
      stream->visitor()->OnCanRead();
    }
    */
    QUIC_LOG(WARNING) << "PushWeTransportSessionVisitor: ignoring incoming bidirectional stream";
  }

  void OnIncomingUnidirectionalStreamAvailable() override {
    /*
    ignore.  we're push-unidirectional-only

    while (true) {
      WebTransportStream* stream =
          session_->AcceptIncomingUnidirectionalStream();
      if (stream == nullptr) {
        return;
      }
      QUIC_DVLOG(1)
          << "PushWebTransportSessionVisitor received a unidirectional stream";
      stream->SetVisitor(
          std::make_unique<WebTransportUnidirectionalPushReadVisitor>(
              stream));
      stream->visitor()->OnCanRead();
    }
    */
    QUIC_LOG(WARNING) << "PushWeTransportSessionVisitor: ignoring incoming unidirectional stream";
  }

  void OnDatagramReceived(absl::string_view datagram) override {
    /*
    ignore.  we're push-unidirectional-only

    quiche::QuicheMemSlice slice(
        quiche::QuicheBuffer::Copy(&allocator_, datagram));
    session_->SendOrQueueDatagram(std::move(slice));
    */
    QUIC_LOG(WARNING) << "PushWeTransportSessionVisitor: ignoring incoming datagram";
  }

  void OnCanCreateNewOutgoingBidirectionalStream() override {
    // QUIC_LOG(WARNING) << "PushWeTransportSessionVisitor: ignoring incoming datagram";
  }
  void OnCanCreateNewOutgoingUnidirectionalStream() override {
    TrySendingUnidirectionalStreams();
  }

  void TrySendingUnidirectionalStreams() {
    while (!streams_to_push_.empty() &&
           session_->CanOpenNextOutgoingUnidirectionalStream()) {
      QUIC_DVLOG(1)
          << "PushWebTransportServer sending a unidirectional stream";
      WebTransportStream* stream = session_->OpenOutgoingUnidirectionalStream();
      stream->SetVisitor(
          std::make_unique<WebTransportUnidirectionalPushWriteVisitor>(
              stream, streams_to_push_.front()));
      streams_to_push_.pop_front();
      stream->visitor()->OnCanWrite();
    }
  }

  void QueuePushUnidirectionalStream(std::string data) {
    streams_to_push_.push_back(std::move(data));
    TrySendingUnidirectionalStreams();
  }

 private:
  QuicPusherPool* pusher_pool_;
  WebTransportSession* session_;
  quiche::SimpleBufferAllocator allocator_;

  quiche::QuicheCircularDeque<std::string> streams_to_push_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_TOOLS_WEB_TRANSPORT_PUSH_VISITORS_H_
