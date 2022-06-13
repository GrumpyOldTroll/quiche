// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "quic/core/crypto/quic_mc_decrypter.h"

#include "absl/strings/string_view.h"
#include "quic/core/crypto/quic_crypter.h"
#include "quic/core/quic_data_reader.h"
#include "quic/core/quic_packets.h"
#include "quic/platform/api/quic_export.h"

namespace quic {

std::unique_ptr<QuicMcDecrypter> QuicMcDecrypter::Create(const ParsedQuicVersion& version,
                                                         QuicTag algorithm) {
  return std::make_unique<QuicMcDecrypter>(version, algorithm);
}

bool QuicMcDecrypter::SetKeyAndIVForPacketRange(absl::string_view key,
                                                absl::string_view iv,
                                                QuicPacketCount from_packet_number,
                                                QuicPacketCount until_packet_number) {
  std::unique_ptr<QuicDecrypter> decrypter = QuicDecrypter::Create(version_, alg_);
  if (decrypter == nullptr) {
    return false;
  }

  if (until_packet_number == 0) {
    decrypters_.erase(decrypters_.lower_bound(from_packet_number), decrypters_.end());
  }
  else {
    auto it = decrypters_.upper_bound(until_packet_number);
    if (it != decrypters_.begin()) {
      it--;
      decrypters_[until_packet_number + 1] = std::move(it->second);
    }
    decrypters_.erase(decrypters_.lower_bound(from_packet_number),
                      decrypters_.upper_bound(until_packet_number));
  }

  decrypter->SetHeaderProtectionKey(header_key_);
  decrypter->SetKey(key);
  decrypter->SetIV(iv);
  decrypters_.emplace(from_packet_number, std::move(decrypter));

  return true;
}

void QuicMcDecrypter::DiscardObsoleteKeys(QuicPacketCount before_packet_number) {
  auto it = decrypters_.upper_bound(before_packet_number);
  if (it != decrypters_.begin()) {
    it--;
    decrypters_.erase(decrypters_.begin(), it);
  }
}

bool QuicMcDecrypter::DecryptPacket(uint64_t packet_number,
                                    absl::string_view associated_data,
                                    absl::string_view ciphertext,
                                    char* output,
                                    size_t* output_length,
                                    size_t max_output_length) {
  auto it = decrypters_.lower_bound(packet_number);
  if (it == decrypters_.end() || it->second == nullptr) {
    return 0;
  }
  return it->second->DecryptPacket(packet_number, associated_data, ciphertext, output,
                                   output_length, max_output_length);
}

bool QuicMcDecrypter::SetHeaderProtectionKey(absl::string_view key) {
  header_key_ = key;
  return true;
}

size_t QuicMcDecrypter::GetKeySize() const {
  if (decrypters_.empty() || decrypters_.begin()->second == nullptr) {
    return QuicDecrypter::Create(version_, alg_)->GetKeySize();
  }
  else {
    return decrypters_.begin()->second->GetKeySize();
  }
}

size_t QuicMcDecrypter::GetIVSize() const {
  if (decrypters_.empty() || decrypters_.begin()->second == nullptr) {
    return QuicDecrypter::Create(version_, alg_)->GetIVSize();
  }
  else {
    return decrypters_.begin()->second->GetIVSize();
  }
}

QuicMcDecrypter::QuicMcDecrypter(const ParsedQuicVersion &version,
                                 QuicTag algorithm)
  : version_(version), alg_(algorithm) { }

}  // namespace quic
