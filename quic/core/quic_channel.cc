
#include "quic_channel.h"
#include "third_party/libmcrx/include/mcrx/libmcrx.h"

namespace quic {
  QuicChannel::QuicChannel(
      //QuicConnectionId channel_id,
      const char* source_ip,
      const char* group_ip,
      uint16_t port
      //std::vector<uint16_t> keys,
      //std::vector<uint16_t> aead_algorithms,
      //std::vector<uint16_t> hash_algorithms*/
  ) {
    //this->state_ = QuicChannel::idle;
    //channel_id_ = channel_id;
    this->source_ip_ = source_ip;
    this->group_ip_ = group_ip;
    this->port_ = port;
    //keys_ = keys;
    //aead_algorithms_ = aead_algorithms;
    //hash_algorithms_ = hash_algorithms;*/
    QUIC_LOG(WARNING) << "Created new quic channel object";
  };
  void QuicChannel::join() {
    struct mcrx_ctx* ctx = NULL;
    mcrx_ctx_new(&ctx);

    struct mcrx_subscription_config conf = MCRX_SUBSCRIPTION_CONFIG_INIT;
    mcrx_subscription_config_pton(&conf, this->source_ip_, this->group_ip_);
    conf.port = this->port_;

    struct mcrx_subscription *sub;
    mcrx_subscription_new(ctx, &conf, &sub);

    mcrx_subscription_join(sub);

    QUIC_LOG(WARNING) << "Joined channel";

  }
}
