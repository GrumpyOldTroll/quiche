
#ifndef SRC_QUIC_CHANNEL_H_
#define SRC_QUIC_CHANNEL_H_

#include "quic/core/quic_framer.h"
#include "third_party/libmcrx/include/mcrx/libmcrx.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicChannel {
 public:
  enum State {idle, joining, joined, timedout};

 QuicChannel(
      //QuicConnectionId channel_id,
      const char* source_ip,
      const char* group_ip,
      uint16_t port
      //std::vector<uint16_t> keys,
      //std::vector<uint16_t> aead_algorithms,
      //std::vector<uint16_t> hash_algorithms*/
      );

 void join();

 private:
   //State state_;
   // Immutable
   //QuicConnectionId channel_id_;
   const char* source_ip_;
   const char* group_ip_;
   uint16_t port_;
   // Mutable
   std::vector<uint16_t> keys_;
   std::vector<uint16_t> aead_algorithms_;
   std::vector<uint16_t> hash_algorithms_;
};


}
#endif  // SRC_QUIC_CHANNEL_H_
