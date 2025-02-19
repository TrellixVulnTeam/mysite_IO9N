// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keep_alive_registry/app_keep_alive_types.h"
#include "base/logging.h"
#include "base/notreached.h"

std::ostream& operator<<(std::ostream& out, const KeepAliveOrigin& origin) {
    switch (origin)
    {
    case KeepAliveOrigin::APP_MAIN_MODULE:
        return out << "APP_MAIN_MODULE";
    case KeepAliveOrigin::APP_UI_EXAMPLE:
        return out << "APP_UI_EXAMPLE";
    case KeepAliveOrigin::APP_CONTROLLER:
      return out << "APP_CONTROLLER";
    case KeepAliveOrigin::BROWSER:
      return out << "BROWSER";
    case KeepAliveOrigin::SESSION_RESTORE:
      return out << "SESSION_RESTORE";
    case KeepAliveOrigin::BACKGROUND_MODE_MANAGER:
      return out << "BACKGROUND_MODE_MANAGER";
    case KeepAliveOrigin::BACKGROUND_MODE_MANAGER_STARTUP:
      return out << "BACKGROUND_MODE_MANAGER_STARTUP";
    case KeepAliveOrigin::BACKGROUND_MODE_MANAGER_FORCE_INSTALLED_EXTENSIONS:
      return out << "BACKGROUND_MODE_MANAGER_FORCE_INSTALLED_EXTENSIONS";
    case KeepAliveOrigin::BACKGROUND_SYNC:
      return out << "BACKGROUND_SYNC";
    case KeepAliveOrigin::BROWSING_DATA_LIFETIME_MANAGER:
      return out << "BROWSING_DATA_LIFETIME_MANAGER";
    case KeepAliveOrigin::LOGIN_DISPLAY_HOST_WEBUI:
      return out << "LOGIN_DISPLAY_HOST_WEBUI";
    case KeepAliveOrigin::PIN_MIGRATION:
      return out << "PIN_MIGRATION";
    case KeepAliveOrigin::REMOTE_DEBUGGING:
      return out << "REMOTE_DEBUGGING";
    case KeepAliveOrigin::DEVTOOLS_WINDOW:
      return out << "DEVTOOLS_WINDOW";
    case KeepAliveOrigin::NOTIFICATION:
      return out << "NOTIFICATION";
    case KeepAliveOrigin::PENDING_NOTIFICATION_CLICK_EVENT:
      return out << "PENDING_NOTIFICATION_CLICK_EVENT";
    case KeepAliveOrigin::IN_FLIGHT_PUSH_MESSAGE:
      return out << "IN_FLIGHT_PUSH_MESSAGE";
    case KeepAliveOrigin::APP_LIST_SERVICE_VIEWS:
      return out << "APP_LIST_SERVICE_VIEWS";
    case KeepAliveOrigin::APP_LIST_SHOWER:
      return out << "APP_LIST_SHOWER";
    case KeepAliveOrigin::APP_START_URL_MIGRATION:
      return out << "APP_START_URL_MIGRATION";
    case KeepAliveOrigin::PANEL:
      return out << "PANEL";
    case KeepAliveOrigin::PANEL_VIEW:
      return out << "PANEL_VIEW";
    case KeepAliveOrigin::PROFILE_HELPER:
      return out << "PROFILE_HELPER";
    case KeepAliveOrigin::PROFILE_LOADER:
      return out << "PROFILE_LOADER";
    case KeepAliveOrigin::USER_MANAGER_VIEW:
      return out << "USER_MANAGER_VIEW";
    case KeepAliveOrigin::CREDENTIAL_PROVIDER_SIGNIN_DIALOG:
      return out << "CREDENTIAL_PROVIDER_SIGNIN_DIALOG";
    case KeepAliveOrigin::NATIVE_MESSAGING_HOST_ERROR_REPORT:
      return out << "NATIVE_MESSAGING_HOST_ERROR_REPORT";
  }

  NOTREACHED();
  return out << static_cast<int>(origin);
}

std::ostream& operator<<(std::ostream& out,
                         const KeepAliveRestartOption& restart) {
  switch (restart) {
    case KeepAliveRestartOption::DISABLED:
      return out << "DISABLED";
    case KeepAliveRestartOption::ENABLED:
      return out << "ENABLED";
  }

  NOTREACHED();
  return out << static_cast<int>(restart);
}
