// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_COMMON_PLATFORM_DEFAULT_QUICHE_PLATFORM_IMPL_QUICHE_SYSTEM_EVENT_LOOP_IMPL_H_
#define QUICHE_COMMON_PLATFORM_DEFAULT_QUICHE_PLATFORM_IMPL_QUICHE_SYSTEM_EVENT_LOOP_IMPL_H_

#include "common/platform/api/quiche_export.h"
#include "common/platform/api/quiche_logging.h"

namespace quiche {

inline void QuicheRunSystemEventLoopIterationImpl() {}

class QUICHE_EXPORT_PRIVATE QuicheSystemEventLoopImpl {
 public:
  QuicheSystemEventLoopImpl(std::string context_name) {
    QUICHE_LOG(INFO) << "Starting event loop for " << context_name;
  }
};

}  // namespace quiche

#endif  // QUICHE_COMMON_PLATFORM_DEFAULT_QUICHE_PLATFORM_IMPL_QUICHE_SYSTEM_EVENT_LOOP_IMPL_H_
