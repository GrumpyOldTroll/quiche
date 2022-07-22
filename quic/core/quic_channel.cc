
#include "quic_channel.h"
#include "third_party/libmcrx/include/mcrx/libmcrx.h"

namespace quic {
  QuicChannel::QuicChannel(
      QuicChannelId channel_id,
      QuicIpAddress source_ip,
      QuicIpAddress group_ip,
      uint16_t port,
      // TODO: not needed, just pass to framer
      // TODO: add (multicast) framer object
      QuicAEADAlgorithmId  header_aead_algorithm,
      const uint8_t* header_key,
      QuicAEADAlgorithmId aead_algorithm,
      QuicHashAlgorithmId hash_algorithm,
      uint64_t max_rate,
      uint64_t max_idle_time,
      uint64_t ack_bundle_size
  ) {
    this->state_ = QuicChannel::initialized;
    this->channel_state_sn_ = 0;
    this->limits_sn_ = 0;
    this->channel_key_sn_ = 0;
    this->channel_id_ = channel_id;
    this->source_ip_ = source_ip;
    this->group_ip_ = group_ip;
    this->port_ = port;
    // TODO: Initialize Framer and pass these to it instead
    this->header_aead_algorithm_ = header_aead_algorithm;
    this->header_key_ = header_key;
    this->aead_algorithm_ = aead_algorithm;
    this->hash_algorithm_ = hash_algorithm;
    this->max_rate_ = max_rate;
    this->max_idle_time_ = max_idle_time,
    this->ack_bundle_size_ = ack_bundle_size;

    //Create mcrx context
    struct mcrx_subscription_config conf = MCRX_SUBSCRIPTION_CONFIG_INIT;
    mcrx_subscription_config_pton(&conf, source_ip.ToString().c_str(), group_ip.ToString().c_str());
    conf.port = port;
    this->mcrx_config_ = conf;
    QUIC_LOG(WARNING) << "Created new quic channel object";
  };

  bool QuicChannel::addKey(QuicPacketCount from_packet_number,
                                   const uint8_t* key) {
      if (this->state_ == QuicChannel::initialized) {
          this->state_ =  QuicChannel::unjoined;
      }
      // TODO: Pass key to framer instead
      key_map_.insert(std::make_pair(from_packet_number, key));
      return true;
  }

  void QuicChannel::setChannelPropertiesSn(QuicChannelKeySequenceNumber sn) {
      this->channel_key_sn_ = sn;
  }

  QuicChannelId QuicChannel::getChannelId(){
      return this->channel_id_;
  }

  QuicChannel::State QuicChannel::getState(){
      return this->state_;
  }

  QuicClientChannelStateSequenceNumber QuicChannel::getStateSn(){
      return this->channel_state_sn_;
  }

  QuicClientLimitsSequenceNumber QuicChannel::getClientLimitsSn(){
      return this->limits_sn_;
  }

  QuicChannelKeySequenceNumber QuicChannel::getChannelKeySn(){
      return this->channel_key_sn_;
  }

  bool QuicChannel::join() {
        struct mcrx_ctx* ctx = NULL;
        mcrx_ctx_new(&ctx);

        struct mcrx_subscription *sub;
        mcrx_subscription_new(ctx, &this->mcrx_config_, &sub);

        mcrx_subscription_join(sub);

        this->state_ = QuicChannel::joined;
        this->mcrx_subscription_ = sub;
        QUIC_LOG(WARNING) << "Joined channel: " << sub;
        return true;
  }

  bool QuicChannel::leave() {
      //TODO: This seems to throw a "libmcrx: mcrx_prv_remove_socket_cb: close() error: Bad file descriptor" error, but the fd is the same as on the join?
      mcrx_subscription_leave(this->mcrx_subscription_);
      QUIC_LOG(WARNING) << "Left channel: " << this->mcrx_subscription_;
      this->state_ =  QuicChannel::unjoined;
      return true;
  }

  void QuicChannel::incrementStateSn() {
      this->channel_state_sn_++;
  }

  //TODO: Probably not required as handled by framer now
  const uint8_t* QuicChannel::getKeyForPacket(QuicPacketCount pid) {
      for(KeyMap::iterator it = key_map_.begin(); it != key_map_.end(); ++it) {
          if (it->first<pid) {
              return it->second;
          }
      }
      return 0;
  }
}
