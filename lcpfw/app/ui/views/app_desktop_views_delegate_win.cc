// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/app_desktop_views_delegate.h"

#include "build/build_config.h"
#include "ui/views/buildflags.h"
#include "ui/views/widget/native_widget_aura.h"

#if BUILDFLAG(ENABLE_DESKTOP_AURA)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif


void AppDesktopViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate)
{
#if BUILDFLAG(ENABLE_DESKTOP_AURA)
    // If we already have a native_widget, we don't have to try to come
    // up with one.
    if (params->native_widget)
        return;

    if (params->parent && params->type != views::Widget::InitParams::TYPE_MENU &&
        params->type != views::Widget::InitParams::TYPE_TOOLTIP)
    {
        params->native_widget = new views::NativeWidgetAura(delegate);
    }
    else
    {
        params->native_widget = new views::DesktopNativeWidgetAura(delegate);
    }
#endif
}
