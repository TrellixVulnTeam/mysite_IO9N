// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_PUBLIC_CPP_UKM_ENTRY_BUILDER_H
#define SERVICES_METRICS_PUBLIC_CPP_UKM_ENTRY_BUILDER_H

#include <string>

#include "base/macros.h"
#include "services/metrics/public/cpp/metrics_export.h"
#include "services/metrics/public/cpp/ukm_entry_builder_base.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
//#include "services/metrics/public/mojom/ukm_interface.mojom.h"

namespace ukm {

// A generic builder object for recording entries in a UkmRecorder, when the
// recording code does not statically know the names of the events/metrics.
// Metrics must still be described in ukm.xml, and this will trigger a DCHECK
// if used to record metrics not described there.
//
// Where possible, prefer using generated objects from ukm_builders.h in the
// ukm::builders namespace instead.
//
// The example usage is:
// ukm::UkmEntryBuilder builder(source_id, "PageLoad");
// builder.SetMetric("NavigationStart", navigation_start_time);
// builder.SetMetric("FirstPaint", first_paint_time);
// builder.Record(ukm_recorder);
class METRICS_EXPORT UkmEntryBuilder final
    : public ukm::internal::UkmEntryBuilderBase {
 public:
  UkmEntryBuilder(SourceId source_id, base::StringPiece event_name);
  ~UkmEntryBuilder() override;

  void SetMetric(base::StringPiece metric_name, int64_t value);

 private:
  DISALLOW_COPY_AND_ASSIGN(UkmEntryBuilder);
};

}  // namespace ukm

#endif  // SERVICES_METRICS_PUBLIC_CPP_UKM_ENTRY_BUILDER_H
