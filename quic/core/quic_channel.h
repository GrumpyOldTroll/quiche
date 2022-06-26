
#ifndef SRC_QUIC_CHANNEL_H_
#define SRC_QUIC_CHANNEL_H_

#include "quic/core/quic_framer.h"
#include "third_party/libmcrx/include/mcrx/libmcrx.h"
#include "absl/container/flat_hash_map.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicChannel {
  protected:
    using KeyMap =
            absl::flat_hash_map<QuicPacketCount, const uint8_t*>;
 public:
  enum State {initialized, unjoined, joined, retired};

 QuicChannel(
      QuicChannelId channel_id,
      QuicIpAddress source_ip,
      QuicIpAddress group_ip,
      uint16_t port,
      QuicAEADAlgorithmId  header_aead_algorithm,
      const uint8_t* header_key,
      QuicAEADAlgorithmId aead_algorithm,
      QuicHashAlgorithmId hash_algorithm,
      uint64_t max_rate,
      uint64_t max_idle_time,
      uint64_t ack_bundle_size
      );

 bool addKey( QuicPacketCount from_packet_number,
                      const uint8_t* key);
 void setChannelPropertiesSn(QuicChannelKeySequenceNumber sn);
 QuicChannelId getChannelId();
 State getState();
 QuicClientChannelStateSequenceNumber getStateSn();
 QuicClientLimitsSequenceNumber getClientLimitsSn();
 QuicChannelKeySequenceNumber getChannelKeySn();
 bool join();
 bool leave();
 void incrementStateSn();
 const uint8_t* getKeyForPacket(QuicPacketCount);

 private:
   State state_;
   struct mcrx_subscription* mcrx_subscription_;
   struct mcrx_subscription_config mcrx_config_;
   QuicClientChannelStateSequenceNumber channel_state_sn_;
   QuicClientLimitsSequenceNumber limits_sn_;
   QuicChannelKeySequenceNumber channel_key_sn_;
   // Immutable
   QuicChannelId channel_id_;
   QuicIpAddress source_ip_;
   QuicIpAddress group_ip_;
   uint16_t port_;
   QuicAEADAlgorithmId  header_aead_algorithm_;
   const uint8_t* header_key_;
   QuicAEADAlgorithmId aead_algorithm_;
   QuicHashAlgorithmId hash_algorithm_;
   uint64_t max_rate_;
   uint64_t max_idle_time_;
   uint64_t ack_bundle_size_;
   // Mutable
   KeyMap key_map_;
};


}
#endif  // SRC_QUIC_CHANNEL_H_
