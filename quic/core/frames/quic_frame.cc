// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/frames/quic_frame.h"

#include "quic/core/frames/quic_new_connection_id_frame.h"
#include "quic/core/frames/quic_retire_connection_id_frame.h"
#include "quic/core/quic_constants.h"
#include "quic/core/quic_types.h"
#include "quic/platform/api/quic_bug_tracker.h"
#include "quic/platform/api/quic_logging.h"
#include "common/platform/api/quiche_mem_slice.h"
#include "common/quiche_buffer_allocator.h"

namespace quic {

QuicFrame::QuicFrame() {}

QuicFrame::QuicFrame(QuicPaddingFrame padding_frame)
    : padding_frame(padding_frame) {}

QuicFrame::QuicFrame(QuicStreamFrame stream_frame)
    : stream_frame(stream_frame) {}

QuicFrame::QuicFrame(QuicHandshakeDoneFrame handshake_done_frame)
    : handshake_done_frame(handshake_done_frame) {}

QuicFrame::QuicFrame(QuicCryptoFrame* crypto_frame)
    : type(CRYPTO_FRAME), crypto_frame(crypto_frame) {}

QuicFrame::QuicFrame(QuicAckFrame* frame) : type(ACK_FRAME), ack_frame(frame) {}

QuicFrame::QuicFrame(QuicMtuDiscoveryFrame frame)
    : mtu_discovery_frame(frame) {}

QuicFrame::QuicFrame(QuicStopWaitingFrame frame) : stop_waiting_frame(frame) {}

QuicFrame::QuicFrame(QuicPingFrame frame) : ping_frame(frame) {}

QuicFrame::QuicFrame(QuicRstStreamFrame* frame)
    : type(RST_STREAM_FRAME), rst_stream_frame(frame) {}

QuicFrame::QuicFrame(QuicConnectionCloseFrame* frame)
    : type(CONNECTION_CLOSE_FRAME), connection_close_frame(frame) {}

QuicFrame::QuicFrame(QuicGoAwayFrame* frame)
    : type(GOAWAY_FRAME), goaway_frame(frame) {}

QuicFrame::QuicFrame(QuicWindowUpdateFrame frame)
    : window_update_frame(frame) {}

QuicFrame::QuicFrame(QuicBlockedFrame frame) : blocked_frame(frame) {}

QuicFrame::QuicFrame(QuicNewConnectionIdFrame* frame)
    : type(NEW_CONNECTION_ID_FRAME), new_connection_id_frame(frame) {}

QuicFrame::QuicFrame(QuicRetireConnectionIdFrame* frame)
    : type(RETIRE_CONNECTION_ID_FRAME), retire_connection_id_frame(frame) {}

QuicFrame::QuicFrame(QuicMaxStreamsFrame frame) : max_streams_frame(frame) {}

QuicFrame::QuicFrame(QuicStreamsBlockedFrame frame)
    : streams_blocked_frame(frame) {}

QuicFrame::QuicFrame(QuicPathResponseFrame* frame)
    : type(PATH_RESPONSE_FRAME), path_response_frame(frame) {}

QuicFrame::QuicFrame(QuicPathChallengeFrame* frame)
    : type(PATH_CHALLENGE_FRAME), path_challenge_frame(frame) {}

QuicFrame::QuicFrame(QuicStopSendingFrame frame) : stop_sending_frame(frame) {}

QuicFrame::QuicFrame(QuicMessageFrame* frame)
    : type(MESSAGE_FRAME), message_frame(frame) {}

QuicFrame::QuicFrame(QuicNewTokenFrame* frame)
    : type(NEW_TOKEN_FRAME), new_token_frame(frame) {}

QuicFrame::QuicFrame(QuicAckFrequencyFrame* frame)
    : type(ACK_FREQUENCY_FRAME), ack_frequency_frame(frame) {}

QuicFrame::QuicFrame(QuicMcChannelAnnounceFrame* frame)
    : type(MC_CHANNEL_ANNOUNCE_FRAME), mc_channel_announce_frame(frame) {
}

QuicFrame::QuicFrame(QuicMcChannelPropertiesFrame* frame)
    : type(MC_CHANNEL_PROPERTIES_FRAME), mc_channel_properties_frame(frame) {}

QuicFrame::QuicFrame(QuicMcChannelJoinFrame* frame)
    : type(MC_CHANNEL_JOIN_FRAME), mc_channel_join_frame(frame) {}

QuicFrame::QuicFrame(QuicMcChannelLeaveFrame* frame)
    : type(MC_CHANNEL_LEAVE_FRAME), mc_channel_leave_frame(frame) {}

QuicFrame::QuicFrame(QuicMcChannelRetireFrame* frame)
    : type(MC_CHANNEL_RETIRE_FRAME), mc_channel_retire_frame(frame) {}

QuicFrame::QuicFrame(QuicMcClientChannelStateFrame* frame)
    : type(MC_CLIENT_CHANNEL_STATE_FRAME), mc_client_channel_state_frame(frame) {}

QuicFrame::QuicFrame(QuicMcClientLimitsFrame* frame)
    : type(MC_CLIENT_LIMITS_FRAME), mc_client_limits_frame(frame) {}

void DeleteFrames(QuicFrames* frames) {
  for (QuicFrame& frame : *frames) {
    DeleteFrame(&frame);
  }
  frames->clear();
}

void DeleteFrame(QuicFrame* frame) {
#if QUIC_FRAME_DEBUG
  // If the frame is not inlined, check that it can be safely deleted.
  if (frame->type != PADDING_FRAME && frame->type != MTU_DISCOVERY_FRAME &&
      frame->type != PING_FRAME && frame->type != MAX_STREAMS_FRAME &&
      frame->type != STOP_WAITING_FRAME &&
      frame->type != STREAMS_BLOCKED_FRAME && frame->type != STREAM_FRAME &&
      frame->type != HANDSHAKE_DONE_FRAME &&
      frame->type != WINDOW_UPDATE_FRAME && frame->type != BLOCKED_FRAME &&
      frame->type != STOP_SENDING_FRAME) {
    QUICHE_CHECK(!frame->delete_forbidden) << *frame;
  }
#endif  // QUIC_FRAME_DEBUG
  switch (frame->type) {
    // Frames smaller than a pointer are inlined, so don't need to be deleted.
    case PADDING_FRAME:
    case MTU_DISCOVERY_FRAME:
    case PING_FRAME:
    case MAX_STREAMS_FRAME:
    case STOP_WAITING_FRAME:
    case STREAMS_BLOCKED_FRAME:
    case STREAM_FRAME:
    case HANDSHAKE_DONE_FRAME:
    case WINDOW_UPDATE_FRAME:
    case BLOCKED_FRAME:
    case STOP_SENDING_FRAME:
      break;
    case ACK_FRAME:
      delete frame->ack_frame;
      break;
    case RST_STREAM_FRAME:
      delete frame->rst_stream_frame;
      break;
    case CONNECTION_CLOSE_FRAME:
      delete frame->connection_close_frame;
      break;
    case GOAWAY_FRAME:
      delete frame->goaway_frame;
      break;
    case PATH_CHALLENGE_FRAME:
      delete frame->path_challenge_frame;
      break;
    case NEW_CONNECTION_ID_FRAME:
      delete frame->new_connection_id_frame;
      break;
    case RETIRE_CONNECTION_ID_FRAME:
      delete frame->retire_connection_id_frame;
      break;
    case PATH_RESPONSE_FRAME:
      delete frame->path_response_frame;
      break;
    case MESSAGE_FRAME:
      delete frame->message_frame;
      break;
    case CRYPTO_FRAME:
      delete frame->crypto_frame;
      break;
    case NEW_TOKEN_FRAME:
      delete frame->new_token_frame;
      break;
    case ACK_FREQUENCY_FRAME:
      delete frame->ack_frequency_frame;
      break;
    case MC_CHANNEL_ANNOUNCE_FRAME:
      delete frame->mc_channel_announce_frame;
      break;
    case MC_CHANNEL_PROPERTIES_FRAME:
      delete frame->mc_channel_properties_frame;
      break;
    case MC_CHANNEL_JOIN_FRAME:
      delete frame->mc_channel_join_frame;
      break;
    case MC_CHANNEL_LEAVE_FRAME:
      delete frame->mc_channel_leave_frame;
      break;
    case MC_CHANNEL_RETIRE_FRAME:
      delete frame->mc_channel_retire_frame;
      break;
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      delete frame->mc_client_channel_state_frame;
      break;
    case MC_CLIENT_LIMITS_FRAME:
      delete frame->mc_client_limits_frame;
      break;
    case MC_CHANNEL_INTEGRITY_FRAMEX:
    case MC_PATH_RESPONSE_FRAMEX:
      break;

    case NUM_FRAME_TYPES:
      QUICHE_DCHECK(false) << "Cannot delete type: " << frame->type;
  }
}

void RemoveFramesForStream(QuicFrames* frames, QuicStreamId stream_id) {
  auto it = frames->begin();
  while (it != frames->end()) {
    if (it->type != STREAM_FRAME || it->stream_frame.stream_id != stream_id) {
      ++it;
      continue;
    }
    it = frames->erase(it);
  }
}

bool IsControlFrame(QuicFrameType type) {
  switch (type) {
    case RST_STREAM_FRAME:
    case GOAWAY_FRAME:
    case WINDOW_UPDATE_FRAME:
    case BLOCKED_FRAME:
    case STREAMS_BLOCKED_FRAME:
    case MAX_STREAMS_FRAME:
    case PING_FRAME:
    case STOP_SENDING_FRAME:
    case NEW_CONNECTION_ID_FRAME:
    case RETIRE_CONNECTION_ID_FRAME:
    case HANDSHAKE_DONE_FRAME:
    case ACK_FREQUENCY_FRAME:
    case NEW_TOKEN_FRAME:
    case MC_CHANNEL_ANNOUNCE_FRAME:
    case MC_CHANNEL_PROPERTIES_FRAME:
    case MC_CHANNEL_JOIN_FRAME:
    case MC_CHANNEL_LEAVE_FRAME:
    // case MC_CHANNEL_INTEGRITY_FRAMEX:
    // case MC_PATH_RESPONSE_FRAMEX:
    case MC_CLIENT_LIMITS_FRAME:
    case MC_CHANNEL_RETIRE_FRAME:
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      return true;
    default:
      return false;
  }
}

QuicControlFrameId GetControlFrameId(const QuicFrame& frame) {
  switch (frame.type) {
    case RST_STREAM_FRAME:
      return frame.rst_stream_frame->control_frame_id;
    case GOAWAY_FRAME:
      return frame.goaway_frame->control_frame_id;
    case WINDOW_UPDATE_FRAME:
      return frame.window_update_frame.control_frame_id;
    case BLOCKED_FRAME:
      return frame.blocked_frame.control_frame_id;
    case STREAMS_BLOCKED_FRAME:
      return frame.streams_blocked_frame.control_frame_id;
    case MAX_STREAMS_FRAME:
      return frame.max_streams_frame.control_frame_id;
    case PING_FRAME:
      return frame.ping_frame.control_frame_id;
    case STOP_SENDING_FRAME:
      return frame.stop_sending_frame.control_frame_id;
    case NEW_CONNECTION_ID_FRAME:
      return frame.new_connection_id_frame->control_frame_id;
    case RETIRE_CONNECTION_ID_FRAME:
      return frame.retire_connection_id_frame->control_frame_id;
    case HANDSHAKE_DONE_FRAME:
      return frame.handshake_done_frame.control_frame_id;
    case ACK_FREQUENCY_FRAME:
      return frame.ack_frequency_frame->control_frame_id;
    case NEW_TOKEN_FRAME:
      return frame.new_token_frame->control_frame_id;
    case MC_CHANNEL_ANNOUNCE_FRAME:
      return frame.mc_channel_announce_frame->control_frame_id;
    case MC_CHANNEL_PROPERTIES_FRAME:
      return frame.mc_channel_properties_frame->control_frame_id;
    case MC_CHANNEL_JOIN_FRAME:
      return frame.mc_channel_join_frame->control_frame_id;
    case MC_CHANNEL_LEAVE_FRAME:
      return frame.mc_channel_leave_frame->control_frame_id;
    case MC_CHANNEL_RETIRE_FRAME:
      return frame.mc_channel_retire_frame->control_frame_id;
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      return frame.mc_client_channel_state_frame->control_frame_id;
    case MC_CLIENT_LIMITS_FRAME:
      return frame.mc_client_limits_frame->control_frame_id;
    case MC_CHANNEL_INTEGRITY_FRAMEX:
    case MC_PATH_RESPONSE_FRAMEX:
      return kInvalidControlFrameId;
    default:
      return kInvalidControlFrameId;
  }
}

void SetControlFrameId(QuicControlFrameId control_frame_id, QuicFrame* frame) {
  switch (frame->type) {
    case RST_STREAM_FRAME:
      frame->rst_stream_frame->control_frame_id = control_frame_id;
      return;
    case GOAWAY_FRAME:
      frame->goaway_frame->control_frame_id = control_frame_id;
      return;
    case WINDOW_UPDATE_FRAME:
      frame->window_update_frame.control_frame_id = control_frame_id;
      return;
    case BLOCKED_FRAME:
      frame->blocked_frame.control_frame_id = control_frame_id;
      return;
    case PING_FRAME:
      frame->ping_frame.control_frame_id = control_frame_id;
      return;
    case STREAMS_BLOCKED_FRAME:
      frame->streams_blocked_frame.control_frame_id = control_frame_id;
      return;
    case MAX_STREAMS_FRAME:
      frame->max_streams_frame.control_frame_id = control_frame_id;
      return;
    case STOP_SENDING_FRAME:
      frame->stop_sending_frame.control_frame_id = control_frame_id;
      return;
    case NEW_CONNECTION_ID_FRAME:
      frame->new_connection_id_frame->control_frame_id = control_frame_id;
      return;
    case RETIRE_CONNECTION_ID_FRAME:
      frame->retire_connection_id_frame->control_frame_id = control_frame_id;
      return;
    case HANDSHAKE_DONE_FRAME:
      frame->handshake_done_frame.control_frame_id = control_frame_id;
      return;
    case ACK_FREQUENCY_FRAME:
      frame->ack_frequency_frame->control_frame_id = control_frame_id;
      return;
    case NEW_TOKEN_FRAME:
      frame->new_token_frame->control_frame_id = control_frame_id;
      return;
    case MC_CHANNEL_ANNOUNCE_FRAME:
      frame->mc_channel_announce_frame->control_frame_id = control_frame_id;
      return;
    case MC_CHANNEL_PROPERTIES_FRAME:
      frame->mc_channel_properties_frame->control_frame_id = control_frame_id;
      return;
    case MC_CHANNEL_JOIN_FRAME:
      frame->mc_channel_join_frame->control_frame_id = control_frame_id;
      return;
    case MC_CHANNEL_LEAVE_FRAME:
      frame->mc_channel_leave_frame->control_frame_id = control_frame_id;
      return;
    case MC_CHANNEL_RETIRE_FRAME:
      frame->mc_channel_retire_frame->control_frame_id = control_frame_id;
      return;
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      frame->mc_client_channel_state_frame->control_frame_id = control_frame_id;
      return;
    case MC_CLIENT_LIMITS_FRAME:
      frame->mc_client_limits_frame->control_frame_id = control_frame_id;
      return;
    // case MC_CHANNEL_INTEGRITY_FRAMEX:
    // case MC_PATH_RESPONSE_FRAMEX:
    default:
      QUIC_BUG(quic_bug_12594_1)
          << "Try to set control frame id of a frame without control frame id";
  }
}

QuicFrame CopyRetransmittableControlFrame(const QuicFrame& frame) {
  QuicFrame copy;
  switch (frame.type) {
    case RST_STREAM_FRAME:
      copy = QuicFrame(new QuicRstStreamFrame(*frame.rst_stream_frame));
      break;
    case GOAWAY_FRAME:
      copy = QuicFrame(new QuicGoAwayFrame(*frame.goaway_frame));
      break;
    case WINDOW_UPDATE_FRAME:
      copy = QuicFrame(QuicWindowUpdateFrame(frame.window_update_frame));
      break;
    case BLOCKED_FRAME:
      copy = QuicFrame(QuicBlockedFrame(frame.blocked_frame));
      break;
    case PING_FRAME:
      copy = QuicFrame(QuicPingFrame(frame.ping_frame.control_frame_id));
      break;
    case STOP_SENDING_FRAME:
      copy = QuicFrame(QuicStopSendingFrame(frame.stop_sending_frame));
      break;
    case NEW_CONNECTION_ID_FRAME:
      copy = QuicFrame(
          new QuicNewConnectionIdFrame(*frame.new_connection_id_frame));
      break;
    case RETIRE_CONNECTION_ID_FRAME:
      copy = QuicFrame(
          new QuicRetireConnectionIdFrame(*frame.retire_connection_id_frame));
      break;
    case STREAMS_BLOCKED_FRAME:
      copy = QuicFrame(QuicStreamsBlockedFrame(frame.streams_blocked_frame));
      break;
    case MAX_STREAMS_FRAME:
      copy = QuicFrame(QuicMaxStreamsFrame(frame.max_streams_frame));
      break;
    case HANDSHAKE_DONE_FRAME:
      copy = QuicFrame(
          QuicHandshakeDoneFrame(frame.handshake_done_frame.control_frame_id));
      break;
    case ACK_FREQUENCY_FRAME:
      copy = QuicFrame(new QuicAckFrequencyFrame(*frame.ack_frequency_frame));
      break;
    case NEW_TOKEN_FRAME:
      copy = QuicFrame(new QuicNewTokenFrame(*frame.new_token_frame));
      break;
    case MC_CHANNEL_ANNOUNCE_FRAME:
      copy = QuicFrame(new QuicMcChannelAnnounceFrame(*frame.mc_channel_announce_frame));
      break;
    case MC_CHANNEL_PROPERTIES_FRAME:
      copy = QuicFrame(new QuicMcChannelPropertiesFrame(*frame.mc_channel_properties_frame));
      break;
    case MC_CHANNEL_JOIN_FRAME:
      copy = QuicFrame(new QuicMcChannelJoinFrame(*frame.mc_channel_join_frame));
      break;
    case MC_CHANNEL_LEAVE_FRAME:
      copy = QuicFrame(new QuicMcChannelLeaveFrame(*frame.mc_channel_leave_frame));
      break;
    case MC_CHANNEL_RETIRE_FRAME:
      copy = QuicFrame(new QuicMcChannelRetireFrame(*frame.mc_channel_retire_frame));
      break;
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      copy = QuicFrame(new QuicMcClientChannelStateFrame(*frame.mc_client_channel_state_frame));
      break;
    case MC_CLIENT_LIMITS_FRAME:
      copy = QuicFrame(new QuicMcClientLimitsFrame(*frame.mc_client_limits_frame));
      break;
    // case MC_CHANNEL_INTEGRITY_FRAMEX:
    // case MC_PATH_RESPONSE_FRAMEX:
    default:
      QUIC_BUG(quic_bug_10533_1)
          << "Try to copy a non-retransmittable control frame: " << frame;
      copy = QuicFrame(QuicPingFrame(kInvalidControlFrameId));
      break;
  }
  return copy;
}

QuicFrame CopyQuicFrame(quiche::QuicheBufferAllocator* allocator,
                        const QuicFrame& frame) {
  QuicFrame copy;
  switch (frame.type) {
    case PADDING_FRAME:
      copy = QuicFrame(QuicPaddingFrame(frame.padding_frame));
      break;
    case RST_STREAM_FRAME:
      copy = QuicFrame(new QuicRstStreamFrame(*frame.rst_stream_frame));
      break;
    case CONNECTION_CLOSE_FRAME:
      copy = QuicFrame(
          new QuicConnectionCloseFrame(*frame.connection_close_frame));
      break;
    case GOAWAY_FRAME:
      copy = QuicFrame(new QuicGoAwayFrame(*frame.goaway_frame));
      break;
    case WINDOW_UPDATE_FRAME:
      copy = QuicFrame(QuicWindowUpdateFrame(frame.window_update_frame));
      break;
    case BLOCKED_FRAME:
      copy = QuicFrame(QuicBlockedFrame(frame.blocked_frame));
      break;
    case STOP_WAITING_FRAME:
      copy = QuicFrame(QuicStopWaitingFrame(frame.stop_waiting_frame));
      break;
    case PING_FRAME:
      copy = QuicFrame(QuicPingFrame(frame.ping_frame.control_frame_id));
      break;
    case CRYPTO_FRAME:
      copy = QuicFrame(new QuicCryptoFrame(*frame.crypto_frame));
      break;
    case STREAM_FRAME:
      copy = QuicFrame(QuicStreamFrame(frame.stream_frame));
      break;
    case ACK_FRAME:
      copy = QuicFrame(new QuicAckFrame(*frame.ack_frame));
      break;
    case MTU_DISCOVERY_FRAME:
      copy = QuicFrame(QuicMtuDiscoveryFrame(frame.mtu_discovery_frame));
      break;
    case NEW_CONNECTION_ID_FRAME:
      copy = QuicFrame(
          new QuicNewConnectionIdFrame(*frame.new_connection_id_frame));
      break;
    case MAX_STREAMS_FRAME:
      copy = QuicFrame(QuicMaxStreamsFrame(frame.max_streams_frame));
      break;
    case STREAMS_BLOCKED_FRAME:
      copy = QuicFrame(QuicStreamsBlockedFrame(frame.streams_blocked_frame));
      break;
    case PATH_RESPONSE_FRAME:
      copy = QuicFrame(new QuicPathResponseFrame(*frame.path_response_frame));
      break;
    case PATH_CHALLENGE_FRAME:
      copy = QuicFrame(new QuicPathChallengeFrame(*frame.path_challenge_frame));
      break;
    case STOP_SENDING_FRAME:
      copy = QuicFrame(QuicStopSendingFrame(frame.stop_sending_frame));
      break;
    case MESSAGE_FRAME:
      copy = QuicFrame(new QuicMessageFrame(frame.message_frame->message_id));
      copy.message_frame->data = frame.message_frame->data;
      copy.message_frame->message_length = frame.message_frame->message_length;
      for (const auto& slice : frame.message_frame->message_data) {
        quiche::QuicheBuffer buffer =
            quiche::QuicheBuffer::Copy(allocator, slice.AsStringView());
        copy.message_frame->message_data.push_back(
            quiche::QuicheMemSlice(std::move(buffer)));
      }
      break;
    case NEW_TOKEN_FRAME:
      copy = QuicFrame(new QuicNewTokenFrame(*frame.new_token_frame));
      break;
    case RETIRE_CONNECTION_ID_FRAME:
      copy = QuicFrame(
          new QuicRetireConnectionIdFrame(*frame.retire_connection_id_frame));
      break;
    case HANDSHAKE_DONE_FRAME:
      copy = QuicFrame(
          QuicHandshakeDoneFrame(frame.handshake_done_frame.control_frame_id));
      break;
    case ACK_FREQUENCY_FRAME:
      copy = QuicFrame(new QuicAckFrequencyFrame(*frame.ack_frequency_frame));
      break;
    case MC_CHANNEL_ANNOUNCE_FRAME:
      copy = QuicFrame(new QuicMcChannelAnnounceFrame(*frame.mc_channel_announce_frame));
      break;
    case MC_CHANNEL_PROPERTIES_FRAME:
      copy = QuicFrame(new QuicMcChannelPropertiesFrame(*frame.mc_channel_properties_frame));
      break;
    case MC_CHANNEL_JOIN_FRAME:
      copy = QuicFrame(new QuicMcChannelJoinFrame(*frame.mc_channel_join_frame));
      break;
    case MC_CHANNEL_LEAVE_FRAME:
      copy = QuicFrame(new QuicMcChannelLeaveFrame(*frame.mc_channel_leave_frame));
      break;
    case MC_CHANNEL_RETIRE_FRAME:
      copy = QuicFrame(new QuicMcChannelRetireFrame(*frame.mc_channel_retire_frame));
      break;
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      copy = QuicFrame(new QuicMcClientChannelStateFrame(*frame.mc_client_channel_state_frame));
      break;
    case MC_CLIENT_LIMITS_FRAME:
      copy = QuicFrame(new QuicMcClientLimitsFrame(*frame.mc_client_limits_frame));
      break;
    case MC_CHANNEL_INTEGRITY_FRAMEX:
    case MC_PATH_RESPONSE_FRAMEX:
    default:
      QUIC_BUG(quic_bug_10533_2) << "Cannot copy frame: " << frame;
      copy = QuicFrame(QuicPingFrame(kInvalidControlFrameId));
      break;
  }
  return copy;
}

QuicFrames CopyQuicFrames(quiche::QuicheBufferAllocator* allocator,
                          const QuicFrames& frames) {
  QuicFrames copy;
  for (const auto& frame : frames) {
    copy.push_back(CopyQuicFrame(allocator, frame));
  }
  return copy;
}

std::ostream& operator<<(std::ostream& os, const QuicFrame& frame) {
  switch (frame.type) {
    case PADDING_FRAME: {
      os << "type { PADDING_FRAME } " << frame.padding_frame;
      break;
    }
    case RST_STREAM_FRAME: {
      os << "type { RST_STREAM_FRAME } " << *(frame.rst_stream_frame);
      break;
    }
    case CONNECTION_CLOSE_FRAME: {
      os << "type { CONNECTION_CLOSE_FRAME } "
         << *(frame.connection_close_frame);
      break;
    }
    case GOAWAY_FRAME: {
      os << "type { GOAWAY_FRAME } " << *(frame.goaway_frame);
      break;
    }
    case WINDOW_UPDATE_FRAME: {
      os << "type { WINDOW_UPDATE_FRAME } " << frame.window_update_frame;
      break;
    }
    case BLOCKED_FRAME: {
      os << "type { BLOCKED_FRAME } " << frame.blocked_frame;
      break;
    }
    case STREAM_FRAME: {
      os << "type { STREAM_FRAME } " << frame.stream_frame;
      break;
    }
    case ACK_FRAME: {
      os << "type { ACK_FRAME } " << *(frame.ack_frame);
      break;
    }
    case STOP_WAITING_FRAME: {
      os << "type { STOP_WAITING_FRAME } " << frame.stop_waiting_frame;
      break;
    }
    case PING_FRAME: {
      os << "type { PING_FRAME } " << frame.ping_frame;
      break;
    }
    case CRYPTO_FRAME: {
      os << "type { CRYPTO_FRAME } " << *(frame.crypto_frame);
      break;
    }
    case MTU_DISCOVERY_FRAME: {
      os << "type { MTU_DISCOVERY_FRAME } ";
      break;
    }
    case NEW_CONNECTION_ID_FRAME:
      os << "type { NEW_CONNECTION_ID } " << *(frame.new_connection_id_frame);
      break;
    case RETIRE_CONNECTION_ID_FRAME:
      os << "type { RETIRE_CONNECTION_ID } "
         << *(frame.retire_connection_id_frame);
      break;
    case MAX_STREAMS_FRAME:
      os << "type { MAX_STREAMS } " << frame.max_streams_frame;
      break;
    case STREAMS_BLOCKED_FRAME:
      os << "type { STREAMS_BLOCKED } " << frame.streams_blocked_frame;
      break;
    case PATH_RESPONSE_FRAME:
      os << "type { PATH_RESPONSE } " << *(frame.path_response_frame);
      break;
    case PATH_CHALLENGE_FRAME:
      os << "type { PATH_CHALLENGE } " << *(frame.path_challenge_frame);
      break;
    case STOP_SENDING_FRAME:
      os << "type { STOP_SENDING } " << frame.stop_sending_frame;
      break;
    case MESSAGE_FRAME:
      os << "type { MESSAGE_FRAME }" << *(frame.message_frame);
      break;
    case NEW_TOKEN_FRAME:
      os << "type { NEW_TOKEN_FRAME }" << *(frame.new_token_frame);
      break;
    case HANDSHAKE_DONE_FRAME:
      os << "type { HANDSHAKE_DONE_FRAME } " << frame.handshake_done_frame;
      break;
    case ACK_FREQUENCY_FRAME:
      os << "type { ACK_FREQUENCY_FRAME } " << *(frame.ack_frequency_frame);
      break;
    case MC_CHANNEL_ANNOUNCE_FRAME:
      os << "type { MC_CHANNEL_ANNOUNCE_FRAME } " << *(frame.mc_channel_announce_frame);
      break;
    case MC_CHANNEL_PROPERTIES_FRAME:
      os << "type { MC_CHANNEL_PROPERTIES_FRAME } " << *(frame.mc_channel_properties_frame);
      break;
    case MC_CHANNEL_JOIN_FRAME:
      os << "type { MC_CHANNEL_JOIN_FRAME } " << *(frame.mc_channel_join_frame);
      break;
    case MC_CHANNEL_LEAVE_FRAME:
      os << "type { MC_CHANNEL_LEAVE_FRAME } " << *(frame.mc_channel_leave_frame);
      break;
    case MC_CHANNEL_RETIRE_FRAME:
      os << "type { MC_CHANNEL_RETIRE_FRAME } " << *(frame.mc_channel_retire_frame);
      break;
    case MC_CLIENT_CHANNEL_STATE_FRAME:
      os << "type { MC_CLIENT_CHANNEL_STATE_FRAME } " << *(frame.mc_client_channel_state_frame);
      break;
    case MC_CLIENT_LIMITS_FRAME:
      os << "type { MC_CLIENT_LIMITS_FRAME } " << *(frame.mc_client_limits_frame);
      break;
    case MC_CHANNEL_INTEGRITY_FRAMEX:
    case MC_PATH_RESPONSE_FRAMEX:
    default: {
      QUIC_LOG(ERROR) << "Unknown frame type: " << frame.type;
      break;
    }
  }
  return os;
}

std::string QuicFramesToString(const QuicFrames& frames) {
  std::ostringstream os;
  for (const QuicFrame& frame : frames) {
    os << frame;
  }
  return os.str();
}

}  // namespace quic
