/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
n * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_FRAME_FETCH_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_FRAME_FETCH_CONTEXT_H_

#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_object.mojom-blink.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/loader/base_fetch_context.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/client_hints_preferences.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_parsers.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class ClientHintsPreferences;
class ContentSecurityPolicy;
class CoreProbeSink;
class Document;
class DocumentLoader;
class FrameOrImportedDocument;
class LocalFrame;
class LocalFrameClient;
class ResourceError;
class ResourceResponse;
class Settings;
class WebContentSettingsClient;

class CORE_EXPORT FrameFetchContext final : public BaseFetchContext {
 public:
  static ResourceFetcher* CreateFetcherForCommittedDocument(DocumentLoader&,
                                                            Document&);
  // Used for creating a FrameFetchContext for an imported Document.
  // |document_loader_| will be set to nullptr.
  static ResourceFetcher* CreateFetcherForImportedDocument(Document* document);

  explicit FrameFetchContext(const FrameOrImportedDocument&);
  ~FrameFetchContext() override = default;

  void AddAdditionalRequestHeaders(ResourceRequest&) override;
  base::Optional<ResourceRequestBlockedReason> CanRequest(
      ResourceType type,
      const ResourceRequest& resource_request,
      const KURL& url,
      const ResourceLoaderOptions& options,
      SecurityViolationReportingPolicy reporting_policy,
      ResourceRequest::RedirectStatus redirect_status) const override;
  mojom::FetchCacheMode ResourceRequestCachePolicy(
      const ResourceRequest&,
      ResourceType,
      FetchParameters::DeferOption) const override;
  void DispatchDidChangeResourcePriority(uint64_t identifier,
                                         ResourceLoadPriority,
                                         int intra_priority_value) override;
  void PrepareRequest(ResourceRequest&,
                      const FetchInitiatorInfo&,
                      WebScopedVirtualTimePauser&,
                      ResourceType) override;
  void DispatchWillSendRequest(
      uint64_t identifier,
      const ResourceRequest&,
      const ResourceResponse& redirect_response,
      ResourceType,
      const FetchInitiatorInfo& = FetchInitiatorInfo()) override;
  void DispatchDidReceiveResponse(uint64_t identifier,
                                  const ResourceRequest&,
                                  const ResourceResponse&,
                                  Resource*,
                                  ResourceResponseType) override;
  void DispatchDidReceiveData(uint64_t identifier,
                              const char* data,
                              uint64_t data_length) override;
  void DispatchDidReceiveEncodedData(uint64_t identifier,
                                     size_t encoded_data_length) override;
  void DispatchDidDownloadToBlob(uint64_t identifier, BlobDataHandle*) override;
  void DispatchDidFinishLoading(uint64_t identifier,
                                TimeTicks finish_time,
                                int64_t encoded_data_length,
                                int64_t decoded_body_length,
                                bool should_report_corb_blocking,
                                ResourceResponseType) override;
  void DispatchDidFail(const KURL&,
                       uint64_t identifier,
                       const ResourceError&,
                       int64_t encoded_data_length,
                       bool is_internal_request) override;

  void RecordLoadingActivity(const ResourceRequest&,
                             ResourceType,
                             const AtomicString& fetch_initiator_name) override;
  void DidObserveLoadingBehavior(WebLoadingBehaviorFlag) override;

  void AddResourceTiming(const ResourceTimingInfo&) override;
  bool AllowImage(bool images_enabled, const KURL&) const override;

  void PopulateResourceRequest(ResourceType,
                               const ClientHintsPreferences&,
                               const FetchParameters::ResourceWidth&,
                               ResourceRequest&) override;

  // Exposed for testing.
  void ModifyRequestForCSP(ResourceRequest&);
  void AddClientHintsIfNecessary(const ClientHintsPreferences&,
                                 const FetchParameters::ResourceWidth&,
                                 ResourceRequest&);

  FetchContext* Detach() override;

  void Trace(blink::Visitor*) override;

  ResourceLoadPriority ModifyPriorityForExperiments(
      ResourceLoadPriority) const override;

  bool CalculateIfAdSubresource(const ResourceRequest& resource_request,
                                ResourceType type) override;

 private:
  class FrameConsoleLogger;
  friend class FrameFetchContextTest;

  struct FrozenState;

  // Convenient accessors below can be used to transparently access the
  // relevant document loader or frame in either cases without null-checks.
  //
  // TODO(kinuko): Remove constness, these return non-const members.
  DocumentLoader* GetDocumentLoader() const;
  DocumentLoader* MasterDocumentLoader() const;
  LocalFrame* GetFrame() const;
  LocalFrameClient* GetLocalFrameClient() const;

  // BaseFetchContext overrides:
  KURL GetSiteForCookies() const override;
  scoped_refptr<const SecurityOrigin> GetTopFrameOrigin() const override;
  SubresourceFilter* GetSubresourceFilter() const override;
  PreviewsResourceLoadingHints* GetPreviewsResourceLoadingHints()
      const override;
  bool AllowScriptFromSource(const KURL&) const override;
  bool ShouldBlockRequestByInspector(const KURL&) const override;
  void DispatchDidBlockRequest(const ResourceRequest&,
                               const FetchInitiatorInfo&,
                               ResourceRequestBlockedReason,
                               ResourceType) const override;
  bool ShouldBypassMainWorldCSP() const override;
  bool IsSVGImageChromeClient() const override;
  void CountUsage(WebFeature) const override;
  void CountDeprecation(WebFeature) const override;
  bool ShouldBlockWebSocketByMixedContentCheck(const KURL&) const override;
  std::unique_ptr<WebSocketHandshakeThrottle> CreateWebSocketHandshakeThrottle()
      override;
  bool ShouldBlockFetchByMixedContentCheck(
      mojom::RequestContextType,
      ResourceRequest::RedirectStatus,
      const KURL&,
      SecurityViolationReportingPolicy) const override;
  bool ShouldBlockFetchAsCredentialedSubresource(const ResourceRequest&,
                                                 const KURL&) const override;

  const KURL& Url() const override;
  const SecurityOrigin* GetParentSecurityOrigin() const override;
  const ContentSecurityPolicy* GetContentSecurityPolicy() const override;
  void AddConsoleMessage(ConsoleMessage*) const override;

  WebContentSettingsClient* GetContentSettingsClient() const;
  Settings* GetSettings() const;
  String GetUserAgent() const;
  UserAgentMetadata GetUserAgentMetadata() const;
  const ClientHintsPreferences GetClientHintsPreferences() const;
  float GetDevicePixelRatio() const;
  bool ShouldSendClientHint(mojom::WebClientHintsType,
                            const ClientHintsPreferences&,
                            const WebEnabledClientHints&) const;
  void SetFirstPartyCookie(ResourceRequest&);

  // Returns true if execution of scripts from the url are allowed. Compared to
  // AllowScriptFromSource(), this method does not generate any
  // notification to the |WebContentSettingsClient| that the execution of the
  // script was blocked. This method should be called only when there is a need
  // to check the settings, and where blocked setting doesn't really imply that
  // JavaScript was blocked from being executed.
  bool AllowScriptFromSourceWithoutNotifying(const KURL&) const;

  // Returns true if the origin of |url| is same as the origin of the top level
  // frame's main resource.
  bool IsFirstPartyOrigin(const KURL& url) const;

  CoreProbeSink* Probe() const;

  Member<const FrameOrImportedDocument> frame_or_imported_document_;

  // The value of |save_data_enabled_| is read once per frame from
  // NetworkStateNotifier, which is guarded by a mutex lock, and cached locally
  // here for performance.
  const bool save_data_enabled_;

  // Non-null only when detached.
  Member<const FrozenState> frozen_state_;
};

}  // namespace blink

#endif
