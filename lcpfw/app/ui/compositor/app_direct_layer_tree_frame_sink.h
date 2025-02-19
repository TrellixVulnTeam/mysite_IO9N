// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base/memory/read_only_shared_memory_region.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/service/display/display_client.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "services/viz/public/mojom/compositing/compositor_frame_sink.mojom.h"

namespace viz {
    class Display;
    class FrameSinkManagerImpl;
}  // namespace viz

namespace ui {

    // This class submits compositor frames to an in-process Display, with the
    // client's frame being the root surface of the Display.
    class AppDirectLayerTreeFrameSink : public cc::LayerTreeFrameSink,
        public viz::mojom::CompositorFrameSinkClient,
        public viz::ExternalBeginFrameSourceClient,
        public viz::DisplayClient {
    public:
        // |frame_sink_manager| and |display| must outlive this class.
        AppDirectLayerTreeFrameSink(
            const viz::FrameSinkId& frame_sink_id,
            viz::FrameSinkManagerImpl* frame_sink_manager,
            viz::Display* display,
            scoped_refptr<viz::ContextProvider> context_provider,
            scoped_refptr<viz::RasterContextProvider> worker_context_provider,
            scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
            gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager);

        AppDirectLayerTreeFrameSink(const AppDirectLayerTreeFrameSink& other) = delete;
        AppDirectLayerTreeFrameSink& operator=(const AppDirectLayerTreeFrameSink& other) =
            delete;

        ~AppDirectLayerTreeFrameSink() override;

        // cc::LayerTreeFrameSink implementation.
        bool BindToClient(cc::LayerTreeFrameSinkClient* client) override;
        void DetachFromClient() override;
        void SubmitCompositorFrame(viz::CompositorFrame frame,
            bool hit_test_data_changed,
            bool show_hit_test_borders) override;
        void DidNotProduceFrame(const viz::BeginFrameAck& ack) override;
        void DidAllocateSharedBitmap(base::ReadOnlySharedMemoryRegion region,
            const viz::SharedBitmapId& id) override;
        void DidDeleteSharedBitmap(const viz::SharedBitmapId& id) override;

        // viz::DisplayClient implementation.
        void DisplayOutputSurfaceLost() override;
        void DisplayWillDrawAndSwap(
            bool will_draw_and_swap,
            viz::AggregatedRenderPassList* render_passes) override;
        void DisplayDidDrawAndSwap() override {}
        void DisplayDidReceiveCALayerParams(
            const gfx::CALayerParams& ca_layer_params) override
        {
        }
        void DisplayDidCompleteSwapWithSize(const gfx::Size& pixel_size) override {}
        void SetWideColorEnabled(bool enabled) override {}
        void SetPreferredFrameInterval(base::TimeDelta interval) override {}
        base::TimeDelta GetPreferredFrameIntervalForFrameSinkId(
            const viz::FrameSinkId& id,
            viz::mojom::CompositorFrameSinkType* type) override;

    private:
        // viz::mojom::CompositorFrameSinkClient implementation:
        void DidReceiveCompositorFrameAck(
            const std::vector<viz::ReturnedResource>& resources) override;
        void OnBeginFrame(const viz::BeginFrameArgs& args,
            const viz::FrameTimingDetailsMap& timing_details) override;
        void ReclaimResources(
            const std::vector<viz::ReturnedResource>& resources) override;
        void OnBeginFramePausedChanged(bool paused) override;

        // viz::ExternalBeginFrameSourceClient implementation:
        void OnNeedsBeginFrames(bool needs_begin_frames) override;

        void DidReceiveCompositorFrameAckInternal(
            const std::vector<viz::ReturnedResource>& resources);

        // This class is only meant to be used on a single thread.
        THREAD_CHECKER(thread_checker_);

        std::unique_ptr<viz::CompositorFrameSinkSupport> support_;

        bool needs_begin_frames_ = false;
        const viz::FrameSinkId frame_sink_id_;
        viz::FrameSinkManagerImpl* frame_sink_manager_;
        viz::ParentLocalSurfaceIdAllocator parent_local_surface_id_allocator_;
        viz::Display* display_;
        gfx::Size last_swap_frame_size_;
        float device_scale_factor_ = 1.f;
        bool is_lost_ = false;
        std::unique_ptr<viz::ExternalBeginFrameSource> begin_frame_source_;

        viz::HitTestRegionList last_hit_test_data_;

        base::WeakPtrFactory<AppDirectLayerTreeFrameSink> weak_factory_{ this };
    };

}  // namespace ui
