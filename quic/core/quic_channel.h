
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
      Uint16 port,
      std::vector<uint16_t> keys,
      std::vector<uint16_t> aead_algoritms,
      std::vector<uint16_t> hash_algoritms,
      ) {

 }
 private:
   State state;
   QuicConnectionId channel_id;
   QuicIpAddress source_ip;
   QuicIpAddress group_ip;
   Uint16 port;
   std::vector<uint16_t> keys;
   std::vector<uint16_t> aead_algoritms;
   std::vector<uint16_t> hash_algoritms;
};
}
#endif  // SRC_QUIC_CHANNEL_H_
