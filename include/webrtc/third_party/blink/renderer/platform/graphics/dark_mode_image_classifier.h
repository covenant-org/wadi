// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_DARK_MODE_IMAGE_CLASSIFIER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_DARK_MODE_IMAGE_CLASSIFIER_H_

#include <vector>

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class IntRect;

class PLATFORM_EXPORT DarkModeImageClassifier {
  DISALLOW_NEW();

 public:
  DarkModeImageClassifier();
  ~DarkModeImageClassifier() = default;

  // Decides if a dark mode filter should be applied to the image or not.
  // |src_rect| is needed in case of image sprites for the location and
  // size of the smaller images that the sprite holds.
  // For images that come from sprites the |src_rect.X| and |src_rect.Y|
  // can be non-zero. But for normal images they are both zero.
  bool ShouldApplyDarkModeFilterToImage(Image& image,
                                        const FloatRect& src_rect);

  bool ComputeImageFeaturesForTesting(Image& image,
                                      std::vector<float>* features) {
    return ComputeImageFeatures(
        image,
        FloatRect(0, 0, static_cast<float>(image.width()),
                  static_cast<float>(image.height())),
        features);
  }

  void SetRandomGeneratorForTesting() { use_testing_random_generator_ = true; }

  DarkModeClassification ClassifyImageUsingDecisionTreeForTesting(
      const std::vector<float>& features) {
    return ClassifyImageUsingDecisionTree(features);
  }

 private:
  enum class ColorMode { kColor = 0, kGrayscale = 1 };

  // Computes the features vector for a given image.
  bool ComputeImageFeatures(Image&, const FloatRect&, std::vector<float>*);

  // Converts image to SkBitmap and returns true if successful.
  bool GetBitmap(Image&, const FloatRect&, SkBitmap*);

  // Given a SkBitmap, extracts a sample set of pixels (|sampled_pixels|),
  // |transparency_ratio|, and |background_ratio|.
  void GetSamples(const SkBitmap&,
                  std::vector<SkColor>* sampled_pixels,
                  float* transparency_ratio,
                  float* background_ratio);

  // Given |sampled_pixels|, |transparency_ratio|, and |background_ratio| for an
  // image, computes the required |features| for classification.
  void GetFeatures(const std::vector<SkColor>& sampled_pixels,
                   const float transparency_ratio,
                   const float background_ratio,
                   std::vector<float>* features);

  // Makes a decision about the image given its features.
  DarkModeClassification ClassifyImage(const std::vector<float>&);

  // Receives sampled pixels and color mode, and returns the ratio of color
  // buckets count to all possible color buckets. If image is in color, a color
  // bucket is a 4 bit per channel representation of each RGB color, and if it
  // is grayscale, each bucket is a 4 bit representation of luminance.
  float ComputeColorBucketsRatio(const std::vector<SkColor>&, const ColorMode);

  // Gets the |required_samples_count| for a specific |block| of the given
  // SkBitmap, and returns |sampled_pixels| and |transparent_pixels_count|.
  void GetBlockSamples(const SkBitmap&,
                       const IntRect& block,
                       const int required_samples_count,
                       std::vector<SkColor>* sampled_pixels,
                       int* transparent_pixels_count);

  // Given sampled pixels from a block of image and the number of transparent
  // pixels, decides if a block is part of background or or not.
  bool IsBlockBackground(const std::vector<SkColor>&, const int);

  // Returns a random number in range [min, max).
  int GetRandomInt(const int min, const int max);

  // Decides if the filter should be applied to the image or not, only using the
  // decision tree. Returns 'kNotClassified' if decision tree cannot give a
  // trustable answer.
  DarkModeClassification ClassifyImageUsingDecisionTree(
      const std::vector<float>&);

  bool use_testing_random_generator_;
  int testing_random_generator_seed_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_DARK_MODE_IMAGE_CLASSIFIER_H_
