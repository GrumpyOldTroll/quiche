
#ifndef SRC_QUIC_CHANNEL_H_
#define SRC_QUIC_CHANNEL_H_

#include "quic/core/quic_framer.h"

namespace quic {
enum State {idle, joining, joined, timedout};
class QUIC_EXPORT_PRIVATE QuicChannel {
 public:
 QuicChannel(
      State state,
      QuicConnectionId channel_id,
      QuicIpAddress source_ip,
      QuicIpAddress group_ip,
      uint16_t port,
      std::vector<uint16_t> keys,
      std::vector<uint16_t> aead_algorithms,
      std::vector<uint16_t> hash_algorithms
      ) {
   state_ = state;
   channel_id_ = channel_id;
   source_ip_ = source_ip;
   group_ip_ = group_ip;
   port_ = port;
   keys_ = keys;
   aead_algorithms_ = aead_algorithms;
   hash_algorithms_ = hash_algorithms;
   std::cout << "Hello World!";
 }
 private:
   State state_;
   QuicConnectionId channel_id_;
   QuicIpAddress source_ip_;
   QuicIpAddress group_ip_;
   uint16_t port_;
   std::vector<uint16_t> keys_;
   std::vector<uint16_t> aead_algorithms_;
   std::vector<uint16_t> hash_algorithms_;
};
}
#endif  // SRC_QUIC_CHANNEL_H_
