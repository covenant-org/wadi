// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PORTAL_HTML_PORTAL_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PORTAL_HTML_PORTAL_ELEMENT_H_

#include "base/unguessable_token.h"
#include "third_party/blink/public/mojom/portal/portal.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"

namespace blink {

class Document;
class PortalActivateOptions;
class RemoteFrame;
class ScriptState;

// The HTMLPortalElement implements the <portal> HTML element. The portal
// element can be used to embed another top-level browsing context, which can be
// activated using script. The portal element is still under development and not
// part of the HTML standard. It can be enabled by passing
// --enable-features=Portals. See also https://github.com/WICG/portals.
class CORE_EXPORT HTMLPortalElement : public HTMLFrameOwnerElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static HTMLElement* Create(Document&);

  explicit HTMLPortalElement(
      Document& document,
      const base::UnguessableToken& portal_token = base::UnguessableToken(),
      mojom::blink::PortalAssociatedPtr portal_ptr = nullptr);
  ~HTMLPortalElement() override;

  // ScriptWrappable overrides.
  void Trace(Visitor* visitor) override;

  // idl implementation.
  ScriptPromise activate(ScriptState*, PortalActivateOptions*);
  void postMessage(const String& message,
                   const String& target_origin,
                   const Vector<ScriptValue>& transfer,
                   ExceptionState& exception_state);
  void postMessage(const String& message,
                   const WindowPostMessageOptions* options,
                   ExceptionState& exception_state);

  const base::UnguessableToken& GetToken() const { return portal_token_; }

  FrameOwnerElementType OwnerType() const override {
    return FrameOwnerElementType::kPortal;
  }

  bool IsActivating() { return is_activating_; }

 private:
  // Navigates the portal to |url_|.
  void Navigate();

  // Consumes the portal interface. When a Portal is activated, or if the
  // renderer receives a connection error, this function will gracefully
  // terminate the portal interface.
  void ConsumePortal();

  // Node overrides
  InsertionNotificationRequest InsertedInto(ContainerNode&) override;
  void RemovedFrom(ContainerNode&) override;

  // Element overrides
  bool IsURLAttribute(const Attribute&) const override;
  void ParseAttribute(const AttributeModificationParams&) override;
  LayoutObject* CreateLayoutObject(const ComputedStyle&, LegacyLayout) override;

  // HTMLFrameOwnerElement overrides
  ParsedFeaturePolicy ConstructContainerPolicy(Vector<String>*) const override {
    return ParsedFeaturePolicy();
  }
  void AttachLayoutTree(AttachContext& context) override;

  // Uniquely identifies the portal, this token is used by the browser process
  // to reference this portal when communicating with the renderer.
  base::UnguessableToken portal_token_;

  Member<RemoteFrame> portal_frame_;

  // Set to true after activate() is called on the portal. It is set to false
  // right before the promise returned by activate() is resolved or rejected.
  bool is_activating_ = false;

  mojom::blink::PortalAssociatedPtr portal_ptr_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_PORTAL_HTML_PORTAL_ELEMENT_H_
