// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_content_layer_client.h"

#include <algorithm>
#include <cstddef>

#include "cc/paint/paint_op_buffer.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/skia_util.h"

namespace cc {

FakeContentLayerClient::ImageData::ImageData(PaintImage img,
                                             const gfx::Point& point,
                                             const SkSamplingOptions& sampling,
                                             const PaintFlags& flags)
    : image(std::move(img)), point(point), sampling(sampling), flags(flags) {}

FakeContentLayerClient::ImageData::ImageData(PaintImage img,
                                             const gfx::Transform& transform,
                                             const SkSamplingOptions& sampling,
                                             const PaintFlags& flags)
    : image(std::move(img)),
      transform(transform),
      sampling(sampling),
      flags(flags) {}

FakeContentLayerClient::ImageData::ImageData(const ImageData& other) = default;

FakeContentLayerClient::ImageData::~ImageData() = default;

FakeContentLayerClient::FakeContentLayerClient() = default;

FakeContentLayerClient::~FakeContentLayerClient() = default;

gfx::Rect FakeContentLayerClient::PaintableRegion() const {
  CHECK(bounds_set_);
  return gfx::Rect(bounds_);
}

scoped_refptr<DisplayItemList>
FakeContentLayerClient::PaintContentsToDisplayList() {
  auto display_list = base::MakeRefCounted<DisplayItemList>();

  for (RectPaintVector::const_iterator it = draw_rects_.begin();
       it != draw_rects_.end(); ++it) {
    const gfx::RectF& draw_rect = it->first;
    const PaintFlags& flags = it->second;

    display_list->StartPaint();
    display_list->push<DrawRectOp>(gfx::RectFToSkRect(draw_rect), flags);
    display_list->EndPaintOfUnpaired(ToEnclosingRect(draw_rect));
  }

  for (ImageVector::const_iterator it = draw_images_.begin();
       it != draw_images_.end(); ++it) {
    if (!it->transform.IsIdentity()) {
      display_list->StartPaint();
      display_list->push<SaveOp>();
      display_list->push<ConcatOp>(it->transform.GetMatrixAsSkM44());
      display_list->EndPaintOfPairedBegin();
    }

    display_list->StartPaint();
    display_list->push<SaveOp>();
    display_list->push<ClipRectOp>(gfx::RectToSkRect(PaintableRegion()),
                                   SkClipOp::kIntersect, false);
    display_list->push<DrawImageOp>(
        it->image, static_cast<float>(it->point.x()),
        static_cast<float>(it->point.y()),
        SkSamplingOptions(it->flags.getFilterQuality(),
                          SkSamplingOptions::kMedium_asMipmapLinear),
        &it->flags);
    display_list->push<RestoreOp>();
    display_list->EndPaintOfUnpaired(PaintableRegion());

    if (!it->transform.IsIdentity()) {
      display_list->StartPaint();
      display_list->push<RestoreOp>();
      display_list->EndPaintOfPairedEnd();
    }
  }

  if (contains_slow_paths_) {
    // Add 6 slow paths, passing the reporting threshold.
    SkPath path;
    path.addCircle(2, 2, 5);
    path.addCircle(3, 4, 2);
    display_list->StartPaint();
    for (int i = 0; i < 6; ++i) {
      display_list->push<ClipPathOp>(path, SkClipOp::kIntersect, true);
    }
    display_list->EndPaintOfUnpaired(PaintableRegion());
  }

  if (fill_with_nonsolid_color_) {
    gfx::Rect draw_rect = PaintableRegion();
    PaintFlags flags;
    flags.setColor(SK_ColorRED);

    display_list->StartPaint();
    while (!draw_rect.IsEmpty()) {
      display_list->push<DrawIRectOp>(gfx::RectToSkIRect(draw_rect), flags);
      draw_rect.Inset(1, 1);
    }
    display_list->EndPaintOfUnpaired(PaintableRegion());
  }

  if (has_non_aa_paint_) {
    PaintFlags flags;
    flags.setAntiAlias(false);
    display_list->StartPaint();
    display_list->push<DrawRectOp>(SkRect::MakeWH(10, 10), flags);
    display_list->EndPaintOfUnpaired(PaintableRegion());
  }

  if (has_draw_text_op_) {
    display_list->StartPaint();
    display_list->push<DrawTextBlobOp>(
        SkTextBlob::MakeFromString("any", SkFont()), (SkScalar)0, (SkScalar)0, PaintFlags());
    display_list->EndPaintOfUnpaired(PaintableRegion());
  }

  display_list->Finalize();
  return display_list;
}

bool FakeContentLayerClient::FillsBoundsCompletely() const { return false; }

}  // namespace cc
