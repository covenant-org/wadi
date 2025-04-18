// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_NON_TOUCH_ELEMENTS_MEDIA_CONTROLS_NON_TOUCH_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_NON_TOUCH_ELEMENTS_MEDIA_CONTROLS_NON_TOUCH_ELEMENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/modules/media_controls/non_touch/media_controls_non_touch_media_event_listener_observer.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {

class MediaControlsNonTouchImpl;

class MediaControlsNonTouchElement
    : public MediaControlsNonTouchMediaEventListenerObserver {
 public:
  void Trace(blink::Visitor* visitor) override;

  // Non-touch media event listener observer implementation.
  void OnFocusIn() override {}
  void OnTimeUpdate() override {}
  void OnDurationChange() override {}
  void OnPlay() override {}
  void OnPause() override {}
  void OnError() override {}
  void OnLoadedMetadata() override {}
  void OnKeyPress(KeyboardEvent* event) override {}
  void OnKeyDown(KeyboardEvent* event) override {}
  void OnKeyUp(KeyboardEvent* event) override {}

 protected:
  MediaControlsNonTouchElement(MediaControlsNonTouchImpl& media_controls);

 private:
  Member<MediaControlsNonTouchImpl> media_controls_;

  DISALLOW_COPY_AND_ASSIGN(MediaControlsNonTouchElement);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIA_CONTROLS_NON_TOUCH_ELEMENTS_MEDIA_CONTROLS_NON_TOUCH_ELEMENT_H_
