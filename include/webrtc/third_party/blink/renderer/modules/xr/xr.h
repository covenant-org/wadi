// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_H_

#include "device/vr/public/mojom/vr_service.mojom-blink.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/execution_context/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/page/focus_changed_observer.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"
#include "third_party/blink/renderer/modules/xr/xr_session_creation_options.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptPromiseResolver;
class XRFrameProvider;

class XR final : public EventTargetWithInlineData,
                 public ContextLifecycleObserver,
                 public device::mojom::blink::VRServiceClient,
                 public FocusChangedObserver {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(XR);

 public:
  static XR* Create(LocalFrame& frame, int64_t source_id) {
    return MakeGarbageCollected<XR>(frame, source_id);
  }

  explicit XR(LocalFrame& frame, int64_t ukm_source_id_);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(devicechange, kDevicechange)

  ScriptPromise supportsSessionMode(ScriptState*, const String&);
  ScriptPromise requestSession(ScriptState*, const XRSessionCreationOptions*);

  XRFrameProvider* frameProvider();

  const device::mojom::blink::XRDevicePtr& xrDevicePtr() const {
    return device_;
  }
  const device::mojom::blink::XRFrameDataProviderPtr& xrMagicWindowProviderPtr()
      const {
    return magic_window_provider_;
  }
  const device::mojom::blink::XREnvironmentIntegrationProviderAssociatedPtr&
  xrEnvironmentProviderPtr();

  // VRServiceClient overrides.
  void OnDeviceChanged() override;

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  // ContextLifecycleObserver overrides.
  void ContextDestroyed(ExecutionContext*) override;
  void Trace(blink::Visitor*) override;

  // FocusChangedObserver overrides.
  void FocusedFrameChanged() override;
  bool IsFrameFocused();

  int64_t GetSourceId() const { return ukm_source_id_; }

  using EnvironmentProviderErrorCallback = base::OnceCallback<void()>;
  void AddEnvironmentProviderErrorHandler(
      EnvironmentProviderErrorCallback callback);

 private:
  class PendingSessionQuery final
      : public GarbageCollected<PendingSessionQuery> {
    DISALLOW_COPY_AND_ASSIGN(PendingSessionQuery);

   public:
    PendingSessionQuery(ScriptPromiseResolver*, XRSession::SessionMode);
    virtual ~PendingSessionQuery() = default;

    virtual void Trace(blink::Visitor*);

    Member<ScriptPromiseResolver> resolver;
    const XRSession::SessionMode mode;
    bool has_user_activation = false;
  };

  void OnRequestDeviceReturned(device::mojom::blink::XRDevicePtr device);
  void DispatchPendingSessionCalls();

  void DispatchRequestSession(PendingSessionQuery*);
  void OnRequestSessionReturned(PendingSessionQuery*,
                                device::mojom::blink::XRSessionPtr);

  void DispatchSupportsSessionMode(PendingSessionQuery*);
  void OnSupportsSessionReturned(PendingSessionQuery*, bool supports_session);

  void EnsureDevice();
  void ReportImmersiveSupported(bool supported);

  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) override;

  XRSession* CreateSession(
      XRSession::SessionMode mode,
      XRSession::EnvironmentBlendMode blend_mode,
      device::mojom::blink::XRSessionClientRequest client_request,
      device::mojom::blink::VRDisplayInfoPtr display_info);
  XRSession* CreateSensorlessInlineSession();

  void Dispose();

  void OnEnvironmentProviderDisconnect();

  bool pending_device_ = false;

  // Indicates whether use of requestDevice has already been logged.
  bool did_log_requestDevice_ = false;
  bool did_log_returned_device_ = false;
  bool did_log_supports_immersive_ = false;

  // Indicates whether we've already logged a request for an immersive session.
  bool did_log_request_immersive_session_ = false;

  const int64_t ukm_source_id_;

  // Track calls that were made prior to the internal device successfully being
  // queried. Can be removed once the service has been updated to allow the
  // respective calls to be made directly.
  HeapVector<Member<PendingSessionQuery>> pending_mode_queries_;
  HeapVector<Member<PendingSessionQuery>> pending_session_requests_;

  Vector<EnvironmentProviderErrorCallback>
      environment_provider_error_callbacks_;

  Member<XRFrameProvider> frame_provider_;
  HeapHashSet<WeakMember<XRSession>> sessions_;
  device::mojom::blink::VRServicePtr service_;
  device::mojom::blink::XRDevicePtr device_;
  device::mojom::blink::XRFrameDataProviderPtr magic_window_provider_;
  device::mojom::blink::XREnvironmentIntegrationProviderAssociatedPtr
      environment_provider_;
  mojo::Binding<device::mojom::blink::VRServiceClient> binding_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_H_
