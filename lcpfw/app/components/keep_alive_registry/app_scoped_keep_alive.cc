// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keep_alive_registry/app_scoped_keep_alive.h"

#include "components/keep_alive_registry/app_keep_alive_registry.h"
#include "components/keep_alive_registry/app_keep_alive_types.h"

ScopedKeepAlive::ScopedKeepAlive(KeepAliveOrigin origin,
                                 KeepAliveRestartOption restart)
    : origin_(origin), restart_(restart) {
  KeepAliveRegistry::GetInstance()->Register(origin_, restart_);
}

ScopedKeepAlive::~ScopedKeepAlive() {
  KeepAliveRegistry::GetInstance()->Unregister(origin_, restart_);
}
