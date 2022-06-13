// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_QUIC_MC_DECRYPTER_H_
#define QUICHE_QUIC_CORE_QUIC_MC_DECRYPTER_H_

#include <map>

#include "absl/strings/string_view.h"
#include "quic/core/crypto/quic_decrypter.h"
#include "quic/core/quic_versions.h"
#include "quic/core/quic_packets.h"
#include "quic/core/quic_tag.h"

namespace quic {

class QUIC_EXPORT_PRIVATE QuicMcDecrypter {
 public:
  QuicMcDecrypter(const ParsedQuicVersion &version,
                  QuicTag algorithm);
  QuicMcDecrypter(const QuicMcDecrypter &) = delete;
  QuicMcDecrypter& operator=(const QuicMcDecrypter &) = delete;
  virtual ~QuicMcDecrypter() {}

  static std::unique_ptr<QuicMcDecrypter> Create(const ParsedQuicVersion& version,
                                                 QuicTag algorithm);

  // Sets the encryption key and IV. Returns true on success, false on failure.
  virtual bool SetKeyAndIVForPacketRange(absl::string_view key,
                                         absl::string_view iv,
                                         QuicPacketCount from_packet_number,
                                         QuicPacketCount until_packet_number);

  // Discards all keys associated only with packet numbers prior to the given
  // packet number.
  virtual void DiscardObsoleteKeys(QuicPacketCount before_packet_number);

  // Populates |output| with the decrypted |ciphertext| and populates
  // |output_length| with the length.  Returns false if there is an error.
  // |output| size is specified by |max_output_length| and must be
  // at least as large as the ciphertext.
  virtual bool DecryptPacket(uint64_t packet_number,
                             absl::string_view associated_data,
                             absl::string_view ciphertext,
                             char* output,
                             size_t* output_length,
                             size_t max_output_length);

  // Sets the key to use for header protection.
  virtual bool SetHeaderProtectionKey(absl::string_view key);

  // Returns the size in bytes of a key for the algorithm.
  virtual size_t GetKeySize() const;
  // Returns the size in bytes of an IV to use with the algorithm.
  virtual size_t GetIVSize() const;

 private:
  std::map<uint64_t, std::unique_ptr<QuicDecrypter> > decrypters_;
  absl::string_view header_key_;
  ParsedQuicVersion version_;
  QuicTag alg_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_QUIC_MC_DECRYPTER_H_
