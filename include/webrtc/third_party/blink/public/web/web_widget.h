/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_WIDGET_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_WIDGET_H_

#include "base/callback.h"
#include "base/time/time.h"
#include "cc/input/browser_controls_state.h"
#include "cc/paint/paint_canvas.h"
#include "cc/trees/element_id.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_float_size.h"
#include "third_party/blink/public/platform/web_input_event_result.h"
#include "third_party/blink/public/platform/web_menu_source_type.h"
#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_text_input_info.h"
#include "third_party/blink/public/web/web_hit_test_result.h"
#include "third_party/blink/public/web/web_ime_text_span.h"
#include "third_party/blink/public/web/web_range.h"
#include "third_party/blink/public/web/web_text_direction.h"

class SkBitmap;

namespace cc {
struct ApplyViewportChangesArgs;
class AnimationHost;
}

namespace gfx {
class Point;
}

namespace blink {
class WebCoalescedInputEvent;
class WebLayerTreeView;

class WebWidget {
 public:
  // Called during set up of the WebWidget to declare the WebLayerTreeView for
  // the widget to use. This does not pass ownership, but the caller must keep
  // the pointer valid until Close() is called.
  virtual void SetLayerTreeView(WebLayerTreeView*, cc::AnimationHost*) = 0;

  // This method closes and deletes the WebWidget.
  virtual void Close() {}

  // Returns the current size of the WebWidget.
  virtual WebSize Size() { return WebSize(); }

  // Called to resize the WebWidget.
  virtual void Resize(const WebSize&) {}

  // Resizes the unscaled visual viewport. Normally the unscaled visual
  // viewport is the same size as the main frame. The passed size becomes the
  // size of the viewport when unscaled (i.e. scale = 1). This is used to
  // shrink the visible viewport to allow things like the ChromeOS virtual
  // keyboard to overlay over content but allow scrolling it into view.
  virtual void ResizeVisualViewport(const WebSize&) {}

  // Called to notify the WebWidget of entering/exiting fullscreen mode.
  virtual void DidEnterFullscreen() {}
  virtual void DidExitFullscreen() {}

  // TODO(crbug.com/704763): Remove the need for this.
  virtual void SetSuppressFrameRequestsWorkaroundFor704763Only(bool) {}

  // Called to update imperative animation state. This should be called before
  // paint, although the client can rate-limit these calls.
  // |last_frame_time| is in seconds. |record_main_frame_metrics| is true when
  // UMA and UKM metrics should be emitted for animation work.
  virtual void BeginFrame(base::TimeTicks last_frame_time,
                          bool record_main_frame_metrics) {}

  // Called after UpdateAllLifecyclePhases has run in response to a BeginFrame.
  virtual void DidBeginFrame() {}

  // Called when main frame metrics are desired. The local frame's UKM
  // aggregator must be informed that collection is starting for the
  // frame.
  virtual void RecordStartOfFrameMetrics() {}

  // Called when a main frame time metric should be emitted, along with
  // any metrics that depend upon the main frame total time.
  virtual void RecordEndOfFrameMetrics(base::TimeTicks frame_begin_time) {}

  // Methods called to mark the beginning and end of input processing work
  // before rAF scripts are executed. Only called when gathering main frame
  // UMA and UKM. That is, when RecordStartOfFrameMetrics has been called, and
  // before RecordEndOfFrameMetrics has been called. Only implement if the
  // rAF input update will be called as part of a layer tree view main frame
  // update.
  virtual void BeginRafAlignedInput() {}
  virtual void EndRafAlignedInput() {}

  // Called to run through the entire set of document lifecycle phases needed
  // to render a frame of the web widget. This MUST be called before Paint,
  // and it may result in calls to WebViewClient::DidInvalidateRect (for
  // non-composited WebViews).
  // |LifecycleUpdateReason| must be used to indicate the source of the
  // update for the purposes of metrics gathering.
  enum class LifecycleUpdate { kLayout, kPrePaint, kAll };
  // This must be kept coordinated with DocumentLifecycle::LifecycleUpdateReason
  enum class LifecycleUpdateReason { kBeginMainFrame, kTest, kOther };
  virtual void UpdateAllLifecyclePhases(LifecycleUpdateReason reason) {
    UpdateLifecycle(LifecycleUpdate::kAll, reason);
  }

  // UpdateLifecycle is used to update to a specific lifestyle phase, as given
  // by |LifecycleUpdate|. To update all lifecycle phases, use
  // UpdateAllLifecyclePhases.
  // |LifecycleUpdateReason| must be used to indicate the source of the
  // update for the purposes of metrics gathering.
  virtual void UpdateLifecycle(LifecycleUpdate requested_update,
                               LifecycleUpdateReason reason) {}

  // Synchronously performs the complete set of document lifecycle phases,
  // including updates to the compositor state, optionally including
  // rasterization.
  virtual void UpdateAllLifecyclePhasesAndCompositeForTesting(bool do_raster) {}

  // Called to paint the rectangular region within the WebWidget
  // onto the specified canvas at (view_port.x, view_port.y).
  //
  // Before calling PaintContent(), the caller must ensure the lifecycle of the
  // widget's frame is clean by calling UpdateLifecycle(LifecycleUpdate::All).
  // It is okay to call paint multiple times once the lifecycle is clean,
  // assuming no other changes are made to the WebWidget (e.g., once
  // events are processed, it should be assumed that another call to
  // UpdateLifecycle is warranted before painting again). Paints starting from
  // the main LayoutView's property tree state, thus ignoring any transient
  // transormations (e.g. pinch-zoom, dev tools emulation, etc.).
  virtual void PaintContent(cc::PaintCanvas*, const WebRect& view_port) {}

  // This should only be called when isAcceleratedCompositingActive() is true.
  virtual void CompositeAndReadbackAsync(
      base::OnceCallback<void(const SkBitmap&)> callback) {}

  // Runs |callback| after a new frame has been submitted to the display
  // compositor, and the display-compositor has displayed it on screen. Forces a
  // redraw so that a new frame is submitted.
  virtual void RequestPresentationCallbackForTesting(
      base::OnceClosure callback) {}

  // Called to inform the WebWidget of a change in theme.
  // Implementors that cache rendered copies of widgets need to re-render
  // on receiving this message
  virtual void ThemeChanged() {}

  // Do a hit test at given point and return the WebHitTestResult.
  virtual WebHitTestResult HitTestResultAt(const gfx::Point&) = 0;

  // Called to inform the WebWidget of an input event.
  virtual WebInputEventResult HandleInputEvent(const WebCoalescedInputEvent&) {
    return WebInputEventResult::kNotHandled;
  }

  // Send any outstanding touch events. Touch events need to be grouped together
  // and any changes since the last time a touch event is going to be sent in
  // the new touch event.
  virtual WebInputEventResult DispatchBufferedTouchEvents() {
    return WebInputEventResult::kNotHandled;
  }

  // Called to inform the WebWidget of the mouse cursor's visibility.
  virtual void SetCursorVisibilityState(bool is_visible) {}

  // Inform WebWidget fallback cursor mode toggled.
  virtual void OnFallbackCursorModeToggled(bool is_on) {}

  // Applies viewport related properties during a commit from the compositor
  // thread.
  virtual void ApplyViewportChanges(const cc::ApplyViewportChangesArgs& args) {}

  virtual void RecordWheelAndTouchScrollingCount(bool has_scrolled_by_wheel,
                                                 bool has_scrolled_by_touch) {}

  virtual void SendOverscrollEventFromImplSide(
      const gfx::Vector2dF& overscroll_delta,
      cc::ElementId scroll_latched_element_id) {}
  virtual void SendScrollEndEventFromImplSide(
      cc::ElementId scroll_latched_element_id) {}

  // Called to inform the WebWidget that mouse capture was lost.
  virtual void MouseCaptureLost() {}

  // Called to inform the WebWidget that it has gained or lost keyboard focus.
  virtual void SetFocus(bool) {}

  // Returns the anchor and focus bounds of the current selection.
  // If the selection range is empty, it returns the caret bounds.
  virtual bool SelectionBounds(WebRect& anchor, WebRect& focus) const {
    return false;
  }

  // Returns true if the WebWidget is currently animating a GestureFling.
  virtual bool IsFlinging() const { return false; }

  // Returns true if the WebWidget uses GPU accelerated compositing
  // to render its contents.
  virtual bool IsAcceleratedCompositingActive() const { return false; }

  // Returns true if the WebWidget created is of type PepperWidget.
  virtual bool IsPepperWidget() const { return false; }

  // Returns true if the WebWidget created is of type WebFrameWidget.
  virtual bool IsWebFrameWidget() const { return false; }

  // Calling WebWidgetClient::requestPointerLock() will result in one
  // return call to didAcquirePointerLock() or didNotAcquirePointerLock().
  virtual void DidAcquirePointerLock() {}
  virtual void DidNotAcquirePointerLock() {}

  // Pointer lock was held, but has been lost. This may be due to a
  // request via WebWidgetClient::requestPointerUnlock(), or for other
  // reasons such as the user exiting lock, window focus changing, etc.
  virtual void DidLosePointerLock() {}

  // Called by client to request showing the context menu.
  virtual void ShowContextMenu(WebMenuSourceType) {}

  // When the WebWidget is part of a frame tree, returns the active url for
  // main frame of that tree, if the main frame is local in that tree. When
  // the WebWidget is of a different kind (e.g. a popup) it returns the active
  // url for the main frame of the frame tree that spawned the WebWidget, if
  // the main frame is local in that tree. When the relevant main frame is
  // remote in that frame tree, then the url is not known, and an empty url is
  // returned.
  virtual WebURL GetURLForDebugTrace() = 0;

 protected:
  ~WebWidget() = default;
};

}  // namespace blink

#endif
