/* Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <memory>
#include "rtc_base/system/rtc_export.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"

namespace webrtc {

class NvVideoEncoderFactory : public webrtc::VideoEncoderFactory{
 public:
  std::vector<SdpVideoFormat> GetSupportedFormats() const override;

  CodecInfo QueryVideoEncoder(const SdpVideoFormat& format) const override;

  std::unique_ptr<VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) override;

private:
  bool IsFormatSupported(const std::vector<webrtc::SdpVideoFormat>& supportedFormats,
                       const webrtc::SdpVideoFormat& format) const;

};
// Creates a new factory that can create the Nvidia video encoder.
RTC_EXPORT std::unique_ptr<VideoEncoderFactory>
CreateNvVideoEncoderFactory();

}  // namespace webrtc
