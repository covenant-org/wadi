// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PROPERTY_NAME_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PROPERTY_NAME_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/css/css_property_names.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/vector_traits.h"

namespace blink {

// This class may be used to represent the name of any valid CSS property,
// including custom properties.
class CORE_EXPORT CSSPropertyName {
  DISALLOW_NEW();

 public:
  explicit CSSPropertyName(CSSPropertyID property_id)
      : property_id_(property_id) {
    DCHECK_NE(property_id, CSSPropertyID::kInvalid);
    DCHECK_NE(property_id, CSSPropertyID::kVariable);
  }

  explicit CSSPropertyName(const AtomicString& custom_property_name)
      : property_id_(CSSPropertyID::kVariable),
        custom_property_name_(custom_property_name) {
    DCHECK(!custom_property_name.IsNull());
  }

  static base::Optional<CSSPropertyName> From(const String& value) {
    const CSSPropertyID property_id = cssPropertyID(value);
    if (property_id == CSSPropertyID::kInvalid)
      return base::nullopt;
    if (property_id == CSSPropertyID::kVariable)
      return base::make_optional(CSSPropertyName(AtomicString(value)));
    return base::make_optional(CSSPropertyName(property_id));
  }

  bool operator==(const CSSPropertyName&) const;
  bool operator!=(const CSSPropertyName& other) const {
    return !(*this == other);
  }

  CSSPropertyID Id() const { return property_id_; }

  bool IsCustomProperty() const {
    return property_id_ == CSSPropertyID::kVariable;
  }

  AtomicString ToAtomicString() const;

 private:
  CSSPropertyID property_id_;
  AtomicString custom_property_name_;
};

}  // namespace blink

WTF_ALLOW_MOVE_INIT_AND_COMPARE_WITH_MEM_FUNCTIONS(blink::CSSPropertyName)

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PROPERTY_NAME_H_
