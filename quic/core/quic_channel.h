
#ifndef SRC_QUIC_CHANNEL_H_
#define SRC_QUIC_CHANNEL_H_

#include "quic/core/quic_framer.h"
#include "third_party/libmcrx/include/mcrx/libmcrx.h"
#include "absl/container/flat_hash_map.h"

namespace quic {

struct properties {
    const uint8_t* key;
    uint64_t max_rate;
    uint64_t max_idle_time;
    uint64_t ack_bundle_size;
};

class QUIC_EXPORT_PRIVATE QuicChannel {
  protected:
    using PropertiesMap =
            absl::flat_hash_map<QuicPacketCount, properties>;
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
      QuicHashAlgorithmId hash_algorithm
      );

 bool addProperties( QuicPacketCount from_packet_number,
                      const uint8_t* key,
                      uint64_t max_rate,
                      uint64_t max_idle_time,
                      uint64_t ack_bundle_size);
 QuicChannelId getChannelId();
 State getState();
 QuicClientChannelStateSequenceNumber getStateSn();
 bool join();
 bool leave();
 bool incrementStateSn();

 private:
   State state_;
   struct mcrx_subscription* mcrx_subscription_;
   struct mcrx_subscription_config mcrx_config_;
   QuicClientChannelStateSequenceNumber channel_state_sn_;
   //QuicChannelPropertiesSequenceNumber channel_properties_sn;
   // Immutable
   QuicChannelId channel_id_;
   QuicIpAddress source_ip_;
   QuicIpAddress group_ip_;
   uint16_t port_;
   QuicAEADAlgorithmId  header_aead_algorithm_;
   const uint8_t* header_key_;
   QuicAEADAlgorithmId aead_algorithm_;
   QuicHashAlgorithmId hash_algorithm_;
   // Mutable
   PropertiesMap properties_map_;
};


}
#endif  // SRC_QUIC_CHANNEL_H_
