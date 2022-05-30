
#include "quic_channel.h"
#include "third_party/libmcrx/include/mcrx/libmcrx.h"

namespace quic {
  QuicChannel::QuicChannel(
      QuicChannelId channel_id,
      QuicIpAddress source_ip,
      QuicIpAddress group_ip,
      uint16_t port,
      QuicAEADAlgorithmId  header_aead_algorithm,
      const uint8_t* header_key,
      QuicAEADAlgorithmId aead_algorithm,
      QuicHashAlgorithmId hash_algorithm
  ) {
    this->state_ = QuicChannel::initialized;
    this->channel_id_ = channel_id;
    this->source_ip_ = source_ip;
    this->group_ip_ = group_ip;
    this->port_ = port;
    this->header_aead_algorithm_ = header_aead_algorithm;
    this->header_key_ = header_key;
    this->aead_algorithm_ = aead_algorithm;
    this->hash_algorithm_ = hash_algorithm;
    QUIC_LOG(WARNING) << "Created new quic channel object";
  };
  void QuicChannel::join() {
    struct mcrx_ctx* ctx = NULL;
    mcrx_ctx_new(&ctx);

    struct mcrx_subscription_config conf = MCRX_SUBSCRIPTION_CONFIG_INIT;
    mcrx_subscription_config_pton(&conf, this->source_ip_.ToString().c_str(), this->group_ip_.ToString().c_str());
    conf.port = this->port_;

    struct mcrx_subscription *sub;
    mcrx_subscription_new(ctx, &conf, &sub);

    mcrx_subscription_join(sub);

    QUIC_LOG(WARNING) << "Joined channel";

  }
  bool QuicChannel::add_properties(QuicPacketCount from_packet_number,
                                   const uint8_t* key,
                                   uint64_t max_rate,
                                   uint64_t max_idle_time,
                                   uint64_t ack_bundle_size) {
      properties props;
      props.key = key;
      props.max_rate = max_rate;
      props.max_idle_time = max_idle_time;
      props.ack_bundle_size = ack_bundle_size;
      properties_.insert(std::make_pair(from_packet_number, props));
      return true;
  }
}
