// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/app_views_delegate.h"

#include "ui/views/widget/native_widget_mac.h"


AppViewsDelegate::AppViewsDelegate() = default;

AppViewsDelegate::~AppViewsDelegate() = default;

void AppViewsDelegate::OnBeforeWidgetInit(
    Widget::InitParams* params,
    internal::NativeWidgetDelegate* delegate)
{
    if (params->opacity == Widget::InitParams::WindowOpacity::kInferred)
    {
        params->opacity = use_transparent_windows_
            ? Widget::InitParams::WindowOpacity::kTranslucent
            : Widget::InitParams::WindowOpacity::kOpaque;
    }
    // TODO(tapted): This should return a *Desktop*NativeWidgetMac.
    if (!params->native_widget && use_desktop_native_widgets_)
        params->native_widget = new NativeWidgetMac(delegate);
}

ui::ContextFactory* AppViewsDelegate::GetContextFactory()
{
    return context_factory_;
}
