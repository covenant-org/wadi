// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_NAVIGATION_PARAMS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_NAVIGATION_PARAMS_H_

#include <memory>

#include "base/containers/span.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/network/public/mojom/referrer_policy.mojom-shared.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/modules/service_worker/web_service_worker_network_provider.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_content_security_policy.h"
#include "third_party/blink/public/platform/web_content_security_policy_struct.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_http_body.h"
#include "third_party/blink/public/platform/web_navigation_body_loader.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_source_location.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/public/web/web_frame_load_type.h"
#include "third_party/blink/public/web/web_history_item.h"
#include "third_party/blink/public/web/web_navigation_policy.h"
#include "third_party/blink/public/web/web_navigation_timings.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "third_party/blink/public/web/web_triggering_event_info.h"

#if INSIDE_BLINK
#include "base/memory/scoped_refptr.h"
#endif

namespace blink {

class KURL;
class SharedBuffer;
class WebDocumentLoader;

// This structure holds all information collected by Blink when
// navigation is being initiated.
struct BLINK_EXPORT WebNavigationInfo {
  WebNavigationInfo() = default;
  ~WebNavigationInfo() = default;

  // The main resource request.
  WebURLRequest url_request;

  // The frame type. This must not be kNone. See RequestContextFrameType.
  // TODO(dgozman): enforce this is not kNone.
  network::mojom::RequestContextFrameType frame_type =
      network::mojom::RequestContextFrameType::kNone;

  // The navigation type. See WebNavigationType.
  WebNavigationType navigation_type = kWebNavigationTypeOther;

  // The navigation policy. See WebNavigationPolicy.
  WebNavigationPolicy navigation_policy = kWebNavigationPolicyCurrentTab;

  // Whether the frame had a transient user activation
  // at the time this request was issued.
  bool has_transient_user_activation = false;

  // The load type. See WebFrameLoadType.
  WebFrameLoadType frame_load_type = WebFrameLoadType::kStandard;

  // During a history load, a child frame can be initially navigated
  // to an url from the history state. This flag indicates it.
  bool is_history_navigation_in_new_child_frame = false;

  // Whether the navigation is a result of client redirect.
  bool is_client_redirect = false;

  // Whether the navigation initiator frame has the |kSandboxDownloads| bit set
  // in its sandbox flags set.
  bool initiator_frame_has_download_sandbox_flag = false;

  // Whether this is a navigation in the opener frame initiated
  // by the window.open'd frame.
  bool is_opener_navigation = false;

  // Whether the runtime feature
  // |BlockingDownloadsInSandboxWithoutUserActivation| is enabled.
  bool blocking_downloads_in_sandbox_without_user_activation_enabled = false;

  // Event information. See WebTriggeringEventInfo.
  WebTriggeringEventInfo triggering_event_info =
      WebTriggeringEventInfo::kUnknown;

  // If the navigation is a result of form submit, the form element is provided.
  WebFormElement form;

  // The location in the source which triggered the navigation.
  // Used to help web developers understand what caused the navigation.
  WebSourceLocation source_location;

  // The initiator of this navigation used by DevTools.
  WebString devtools_initiator_info;

  // Whether this navigation should check CSP. See
  // WebContentSecurityPolicyDisposition.
  WebContentSecurityPolicyDisposition
      should_check_main_world_content_security_policy =
          kWebContentSecurityPolicyDispositionCheck;

  // When navigating to a blob url, this token specifies the blob.
  mojo::ScopedMessagePipeHandle blob_url_token;

  // When navigation initiated from the user input, this tracks
  // the input start time.
  base::TimeTicks input_start;

  // This is the navigation relevant CSP to be used during request and response
  // checks.
  WebContentSecurityPolicyList initiator_csp;

  // The navigation initiator, if any.
  mojo::ScopedMessagePipeHandle navigation_initiator_handle;

  // Specifies whether or not a MHTML Archive can be used to load a subframe
  // resource instead of doing a network request.
  enum class ArchiveStatus { Absent, Present };
  ArchiveStatus archive_status = ArchiveStatus::Absent;

  // The value of hrefTranslate attribute of a link, if this navigation was
  // inititated by clicking a link.
  WebString href_translate;
};

// This structure holds all information provided by the embedder that is
// needed for blink to load a Document. This is hence different from
// WebDocumentLoader::ExtraData, which is an opaque structure stored in the
// DocumentLoader and used by the embedder.
struct BLINK_EXPORT WebNavigationParams {
  WebNavigationParams();
  ~WebNavigationParams();

  // Allows to specify |devtools_navigation_token|, instead of creating
  // a new one.
  explicit WebNavigationParams(const base::UnguessableToken&);

  // Shortcut for navigating based on WebNavigationInfo parameters.
  static std::unique_ptr<WebNavigationParams> CreateFromInfo(
      const WebNavigationInfo&);

  // Shortcut for loading html with "text/html" mime type and "UTF8" encoding.
  static std::unique_ptr<WebNavigationParams> CreateWithHTMLString(
      base::span<const char> html,
      const WebURL& base_url);

  // Shortcut for loading an error page html.
  static std::unique_ptr<WebNavigationParams> CreateForErrorPage(
      WebDocumentLoader* failed_document_loader,
      base::span<const char> html,
      const WebURL& base_url,
      const WebURL& unreachable_url,
      int error_code);

#if INSIDE_BLINK
  // Shortcut for loading html with "text/html" mime type and "UTF8" encoding.
  static std::unique_ptr<WebNavigationParams> CreateWithHTMLBuffer(
      scoped_refptr<SharedBuffer> buffer,
      const KURL& base_url);
#endif

  // Fills |body_loader| based on the provided static data.
  static void FillBodyLoader(WebNavigationParams*, base::span<const char> data);
  static void FillBodyLoader(WebNavigationParams*, WebData data);

  // Fills |response| and |body_loader| based on the provided static data.
  // |url| must be set already.
  static void FillStaticResponse(WebNavigationParams*,
                                 WebString mime_type,
                                 WebString text_encoding,
                                 base::span<const char> data);

  // This block defines the request used to load the main resource
  // for this navigation.

  // This URL indicates the security origin and will be used as a base URL
  // to resolve links in the committed document.
  WebURL url;
  // The http method (if any) used to load the main resource.
  WebString http_method;
  // The referrer string and policy used to load the main resource.
  WebString referrer;
  network::mojom::ReferrerPolicy referrer_policy =
      network::mojom::ReferrerPolicy::kDefault;
  // The http body of the request used to load the main resource, if any.
  WebHTTPBody http_body;
  // The http content type of the request used to load the main resource, if
  // any.
  WebString http_content_type;
  // The origin policy for this navigation.
  WebString origin_policy;
  // The origin of the request used to load the main resource, specified at
  // https://fetch.spec.whatwg.org/#concept-request-origin. Can be null.
  // TODO(dgozman,nasko): we shouldn't need both this and |origin_to_commit|.
  WebSecurityOrigin requestor_origin;
  // If non-null, used as a URL which we weren't able to load. For example,
  // history item will contain this URL instead of request's URL.
  // This URL can be retrieved through WebDocumentLoader::UnreachableURL.
  WebURL unreachable_url;

  // The net error code for failed navigation. Must be non-zero when
  // |unreachable_url| is non-null.
  int error_code = 0;

  // This block defines the document content. The alternatives in the order
  // of precedence are:
  // 1. If |is_static_data| is false:
  //   1a. If url loads as an empty document (according to
  //       WebDocumentLoader::WillLoadUrlAsEmpty), the document will be empty.
  //   1b. If loading an iframe of mhtml archive, the document will be
  //       retrieved from the archive.
  // 2. Otherwise, provided redirects and response are used to construct
  //    the final response.
  //   2a. If body loader is present, it will be used to fetch the content.
  //   2b. If body loader is missing, but url is a data url, it will be
  //       decoded and used as response and document content.
  //   2c. If decoding data url fails, or url is not a data url, the
  //       navigation will fail.

  struct RedirectInfo {
    // New base url after redirect.
    WebURL new_url;
    // Http method used for redirect.
    WebString new_http_method;
    // New referrer string and policy used for redirect.
    WebString new_referrer;
    network::mojom::ReferrerPolicy new_referrer_policy;
    // Redirect response itself.
    // TODO(dgozman): we only use this response for navigation timings.
    // Perhaps, we can just get rid of it.
    WebURLResponse redirect_response;
  };
  // Redirects which happened while fetching the main resource.
  // TODO(dgozman): we are only interested in the final values instead of
  // all information about redirects.
  WebVector<RedirectInfo> redirects;
  // The final response for the main resource. This will be used to determine
  // the type of resulting document.
  WebURLResponse response;
  // The body loader which allows to retrieve the response body when available.
  std::unique_ptr<WebNavigationBodyLoader> body_loader;
  // Whether |response| and |body_loader| represent static data. In this case
  // we skip some security checks and insist on loading this exact content.
  bool is_static_data = false;

  // This block defines the type of the navigation.

  // The load type. See WebFrameLoadType for definition.
  WebFrameLoadType frame_load_type = WebFrameLoadType::kStandard;
  // History item should be provided for back-forward load types.
  WebHistoryItem history_item;
  // Whether this navigation is a result of client redirect.
  bool is_client_redirect = false;

  // Miscellaneous parameters.

  // The origin in which a navigation should commit. When provided, Blink
  // should use this origin directly and not compute locally the new document
  // origin.
  WebSecurityOrigin origin_to_commit;
  // The devtools token for this navigation. See DocumentLoader
  // for details.
  base::UnguessableToken devtools_navigation_token;
  // Known timings related to navigation. If the navigation has
  // started in another process, timings are propagated from there.
  WebNavigationTimings navigation_timings;
  // Indicates that the frame was previously discarded.
  // was_discarded is exposed on Document after discard, see:
  // https://github.com/WICG/web-lifecycle
  bool was_discarded = false;
  // Whether this navigation had a transient user activation
  // when inititated.
  bool had_transient_activation = false;
  // Whether this navigation has a sticky user activation flag.
  bool is_user_activated = false;
  // The previews state which should be used for this navigation.
  WebURLRequest::PreviewsState previews_state =
      WebURLRequest::kPreviewsUnspecified;
  // The service worker network provider to be used in the new
  // document.
  std::unique_ptr<blink::WebServiceWorkerNetworkProvider>
      service_worker_network_provider;
};

}  // namespace blink

#endif
