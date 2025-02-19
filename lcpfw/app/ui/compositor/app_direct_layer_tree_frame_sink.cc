// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/app_direct_layer_tree_frame_sink.h"

#include <memory>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "cc/trees/layer_tree_frame_sink_client.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface.h"

namespace ui {

    AppDirectLayerTreeFrameSink::AppDirectLayerTreeFrameSink(
        const viz::FrameSinkId& frame_sink_id,
        viz::FrameSinkManagerImpl* frame_sink_manager,
        viz::Display* display,
        scoped_refptr<viz::ContextProvider> context_provider,
        scoped_refptr<viz::RasterContextProvider> worker_context_provider,
        scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
        gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
        : LayerTreeFrameSink(std::move(context_provider),
            std::move(worker_context_provider),
            std::move(compositor_task_runner),
            gpu_memory_buffer_manager),
        frame_sink_id_(frame_sink_id),
        frame_sink_manager_(frame_sink_manager),
        display_(display)
    {
        DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    }

    AppDirectLayerTreeFrameSink::~AppDirectLayerTreeFrameSink()
    {
        DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    }

    bool AppDirectLayerTreeFrameSink::BindToClient(
        cc::LayerTreeFrameSinkClient* client)
    {
        DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

        if (!cc::LayerTreeFrameSink::BindToClient(client))
            return false;

        support_ = std::make_unique<viz::CompositorFrameSinkSupport>(
            this, frame_sink_manager_, frame_sink_id_, /*is_root=*/true);
        begin_frame_source_ = std::make_unique<viz::ExternalBeginFrameSource>(this);
        client_->SetBeginFrameSource(begin_frame_source_.get());

        // Avoid initializing GL context here, as this should be sharing the
        // Display's context.
        display_->Initialize(this, frame_sink_manager_->surface_manager());

        support_->SetUpHitTest(display_);

        return true;
    }

    void AppDirectLayerTreeFrameSink::DetachFromClient()
    {
        client_->SetBeginFrameSource(nullptr);
        begin_frame_source_.reset();

        // Unregister the SurfaceFactoryClient here instead of the dtor so that only
        // one client is alive for this namespace at any given time.
        support_.reset();

        cc::LayerTreeFrameSink::DetachFromClient();
    }

    void AppDirectLayerTreeFrameSink::SubmitCompositorFrame(
        viz::CompositorFrame frame,
        bool hit_test_data_changed,
        bool show_hit_test_borders)
    {
        DCHECK(frame.metadata.begin_frame_ack.has_damage);
        DCHECK(frame.metadata.begin_frame_ack.frame_id.IsSequenceValid());

        if (frame.size_in_pixels() != last_swap_frame_size_ ||
            frame.device_scale_factor() != device_scale_factor_ ||
            !parent_local_surface_id_allocator_.HasValidLocalSurfaceId())
        {
            parent_local_surface_id_allocator_.GenerateId();
            last_swap_frame_size_ = frame.size_in_pixels();
            device_scale_factor_ = frame.device_scale_factor();
            display_->SetLocalSurfaceId(
                parent_local_surface_id_allocator_.GetCurrentLocalSurfaceId(),
                device_scale_factor_);
        }

        base::Optional<viz::HitTestRegionList> hit_test_region_list =
            client_->BuildHitTestData();

        if (!hit_test_region_list)
        {
            last_hit_test_data_ = viz::HitTestRegionList();
        }
        else if (!hit_test_data_changed)
        {
            // Do not send duplicate hit-test data.
            if (viz::HitTestRegionList::IsEqual(*hit_test_region_list,
                last_hit_test_data_))
            {
                DCHECK(!viz::HitTestRegionList::IsEqual(*hit_test_region_list,
                    viz::HitTestRegionList()));
                hit_test_region_list = base::nullopt;
            }
            else
            {
                last_hit_test_data_ = *hit_test_region_list;
            }
        }
        else
        {
            last_hit_test_data_ = *hit_test_region_list;
        }

        support_->SubmitCompositorFrame(
            parent_local_surface_id_allocator_.GetCurrentLocalSurfaceId(),
            std::move(frame), std::move(hit_test_region_list));
    }

    void AppDirectLayerTreeFrameSink::DidNotProduceFrame(
        const viz::BeginFrameAck& ack)
    {
        DCHECK(!ack.has_damage);
        DCHECK(ack.frame_id.IsSequenceValid());
        support_->DidNotProduceFrame(ack);
    }

    void AppDirectLayerTreeFrameSink::DidAllocateSharedBitmap(
        base::ReadOnlySharedMemoryRegion region,
        const viz::SharedBitmapId& id)
    {
        bool ok = support_->DidAllocateSharedBitmap(std::move(region), id);
        DCHECK(ok);
    }

    void AppDirectLayerTreeFrameSink::DidDeleteSharedBitmap(
        const viz::SharedBitmapId& id)
    {
        support_->DidDeleteSharedBitmap(id);
    }

    void AppDirectLayerTreeFrameSink::DisplayOutputSurfaceLost()
    {
        is_lost_ = true;
        client_->DidLoseLayerTreeFrameSink();
    }

    void AppDirectLayerTreeFrameSink::DisplayWillDrawAndSwap(
        bool will_draw_and_swap,
        viz::AggregatedRenderPassList* render_passes)
    {
        if (support_->GetHitTestAggregator())
        {
            support_->GetHitTestAggregator()->Aggregate(display_->CurrentSurfaceId(),
                render_passes);
        }
    }

    base::TimeDelta
        AppDirectLayerTreeFrameSink::GetPreferredFrameIntervalForFrameSinkId(
            const viz::FrameSinkId& id,
            viz::mojom::CompositorFrameSinkType* type)
    {
        return frame_sink_manager_->GetPreferredFrameIntervalForFrameSinkId(id, type);
    }

    void AppDirectLayerTreeFrameSink::DidReceiveCompositorFrameAck(
        const std::vector<viz::ReturnedResource>& resources)
    {
        // Submitting a CompositorFrame can synchronously draw and dispatch a frame
        // ack. PostTask to ensure the client is notified on a new stack frame.
        compositor_task_runner_->PostTask(
            FROM_HERE,
            base::BindOnce(
                &AppDirectLayerTreeFrameSink::DidReceiveCompositorFrameAckInternal,
                weak_factory_.GetWeakPtr(), resources));
    }

    void AppDirectLayerTreeFrameSink::DidReceiveCompositorFrameAckInternal(
        const std::vector<viz::ReturnedResource>& resources)
    {
        client_->ReclaimResources(resources);
        client_->DidReceiveCompositorFrameAck();
    }

    void AppDirectLayerTreeFrameSink::OnBeginFrame(
        const viz::BeginFrameArgs& args,
        const viz::FrameTimingDetailsMap& timing_details)
    {
        for (const auto& pair : timing_details)
            client_->DidPresentCompositorFrame(pair.first, pair.second);

        if (!needs_begin_frames_)
        {
            // OnBeginFrame() can be called just to deliver presentation feedback, so
            // report that we didn't use this BeginFrame.
            DidNotProduceFrame(viz::BeginFrameAck(args, false));
            return;
        }

        begin_frame_source_->OnBeginFrame(args);
    }

    void AppDirectLayerTreeFrameSink::ReclaimResources(
        const std::vector<viz::ReturnedResource>& resources)
    {
        client_->ReclaimResources(resources);
    }

    void AppDirectLayerTreeFrameSink::OnBeginFramePausedChanged(bool paused)
    {
        begin_frame_source_->OnSetBeginFrameSourcePaused(paused);
    }

    void AppDirectLayerTreeFrameSink::OnNeedsBeginFrames(bool needs_begin_frames)
    {
        needs_begin_frames_ = needs_begin_frames;
        support_->SetNeedsBeginFrame(needs_begin_frames);
    }

}  // namespace ui
