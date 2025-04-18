// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_FILESYSTEM_ASYNC_CALLBACK_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_FILESYSTEM_ASYNC_CALLBACK_HELPER_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/file_error.h"
#include "third_party/blink/renderer/modules/filesystem/directory_entry.h"
#include "third_party/blink/renderer/modules/filesystem/dom_file_system.h"
#include "third_party/blink/renderer/modules/filesystem/entry.h"
#include "third_party/blink/renderer/modules/filesystem/file_system_base_handle.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

class AsyncCallbackHelper {
  STATIC_ONLY(AsyncCallbackHelper);

 public:
  template <typename CallbackParam, typename V8Callback>
  static base::OnceCallback<void(CallbackParam*)> SuccessCallback(
      V8Callback* success_callback) {
    auto success_callback_wrapper = WTF::Bind(
        [](V8PersistentCallbackInterface<V8Callback>* persistent_callback,
           CallbackParam* param) {
          persistent_callback->InvokeAndReportException(nullptr, param);
        },
        WrapPersistentIfNeeded(
            ToV8PersistentCallbackInterface(success_callback)));
    return success_callback_wrapper;
  }

  static base::OnceCallback<void(base::File::Error)> ErrorCallback(
      V8ErrorCallback* error_callback) {
    if (!error_callback)
      return base::OnceCallback<void(base::File::Error)>();

    return WTF::Bind(
        [](V8PersistentCallbackInterface<V8ErrorCallback>* persistent_callback,
           base::File::Error error) {
          persistent_callback->InvokeAndReportException(
              nullptr, file_error::CreateDOMException(error));
        },
        WrapPersistentIfNeeded(
            ToV8PersistentCallbackInterface(error_callback)));
  }

  template <typename CallbackParam>
  static base::OnceCallback<void(CallbackParam*)> SuccessPromise(
      ScriptPromiseResolver* resolver) {
    return WTF::Bind(
        [](ScriptPromiseResolver* resolver, CallbackParam* param) {
          resolver->Resolve(param->asFileSystemHandle());
        },
        WrapPersistentIfNeeded(resolver));
  }

  static base::OnceCallback<void(base::File::Error)> ErrorPromise(
      ScriptPromiseResolver* resolver) {
    return WTF::Bind(
        [](ScriptPromiseResolver* resolver, base::File::Error error) {
          resolver->Reject(file_error::CreateDOMException(error));
        },
        WrapPersistentIfNeeded(resolver));
  }

  // The two methods below are not templetized, to be used exclusively for
  // VoidCallbacks.
  static base::OnceCallback<void()> VoidSuccessCallback(
      V8VoidCallback* success_callback) {
    auto success_callback_wrapper = WTF::Bind(
        [](V8PersistentCallbackInterface<V8VoidCallback>* persistent_callback) {
          persistent_callback->InvokeAndReportException(nullptr);
        },
        WrapPersistentIfNeeded(
            ToV8PersistentCallbackInterface(success_callback)));
    return success_callback_wrapper;
  }

  static base::OnceCallback<void()> VoidSuccessPromise(
      ScriptPromiseResolver* resolver) {
    return WTF::Bind(
        [](ScriptPromiseResolver* resolver) { resolver->Resolve(); },
        WrapPersistentIfNeeded(resolver));
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_FILESYSTEM_ASYNC_CALLBACK_HELPER_H_
