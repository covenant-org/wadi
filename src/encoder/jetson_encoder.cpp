#include "encoder/jetson_encoder.h"
#include "absl/memory/memory.h"
#include <memory>

webrtc::VideoEncoderFactory::CodecInfo JetsonEncoderFactory::QueryVideoEncoder(
    const webrtc::SdpVideoFormat &format) const {
  VideoEncoderFactory::CodecInfo info;
  info.has_internal_source = false;
  info.is_hardware_accelerated = true;
  return info;
}

std::unique_ptr<webrtc::VideoEncoder>
JetsonEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format) {
  return absl::make_unique<JetsonEncoder>();
}

std::unique_ptr<webrtc::VideoEncoderFactory> CreateJetsonEncoderFactory() {
  return absl::make_unique<JetsonEncoderFactory>();
}
