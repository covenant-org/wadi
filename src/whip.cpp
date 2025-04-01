#include "whip.h"
#include "HTTPRequest/Request.hpp"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/jsep.h"
#include "api/peer_connection_interface.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "common_types.h"
#include "logging.h"
#include "media/base/video_broadcaster.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "pc/video_track_source.h"
#include "rtc_base/location.h"
#include "v4l.h"
#include <cstdint>
#include <stdexcept>
#include <thread>

webrtc::VideoType fourcc_to_videotype(std::string fourcc) {
  assert(fourcc.length() == 4);
  if (fourcc == "I420" || fourcc == "YU12") {
    return webrtc::VideoType::kI420;
  }
  if (fourcc == "IYUV") {
    return webrtc::VideoType::kIYUV;
  }
  if (fourcc == "RGB24") {
    return webrtc::VideoType::kRGB24;
  }
  if (fourcc == "YUY2") {
    return webrtc::VideoType::kYUY2;
  }
  if (fourcc == "YV12") {
    return webrtc::VideoType::kYV12;
  }
  if (fourcc == "UYVY") {
    return webrtc::VideoType::kUYVY;
  }
  if (fourcc == "NV21") {
    return webrtc::VideoType::kNV21;
  }
  if (fourcc == "NV12") {
    return webrtc::VideoType::kNV12;
  }
  if (fourcc == "BGRA") {
    return webrtc::VideoType::kBGRA;
  }
  return webrtc::VideoType::kUnknown;
}

class CapturerTrackSource : public webrtc::VideoTrackSource,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
  static rtc::scoped_refptr<CapturerTrackSource>
  Create(std::string video_device_path) {
    std::unique_ptr<V4LDevice> device(new V4LDevice(video_device_path));
    tlog("Creating video capturer");
    rtc::scoped_refptr<webrtc::VideoCaptureModule> capturer =
        webrtc::VideoCaptureFactory::Create((const char *)device->cap.bus_info);
    if (!capturer) {
      tlog("Failed to create video capturer");
      return nullptr;
    }
    tlog("Created video capturer");
    return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer),
                                                          std::move(device));
  }

  void OnFrame(const webrtc::VideoFrame &frame) override {
    // tlog("OnFrame");
  }

  void OnDiscardedFrame() override { tlog("OnDiscardedFrame"); }

protected:
  explicit CapturerTrackSource(
      rtc::scoped_refptr<webrtc::VideoCaptureModule> capturer,
      std::unique_ptr<V4LDevice> device)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)),
        device_(std::move(device)) {
    this->capturer_->RegisterCaptureDataCallback(&this->broadcaster_);

    struct webrtc::VideoCaptureCapability capability;
    capability.width = this->device_->fmt.fmt.pix.width;
    capability.height = this->device_->fmt.fmt.pix.height;
    //      capability.maxFPS = this->device_->fmt.fmt.pix.;
    capability.maxFPS = 30;
    capability.videoType = fourcc_to_videotype(
        fourcc_to_string(this->device_->fmt.fmt.pix.pixelformat));
//    int res = this->capturer_->StartCapture(capability);
 //   if (res < 0) {
 //     tlog("Failed to start video capturer");
 //   }
    //    this->broadcaster_.AddOrUpdateSink(this, this->broadcaster_.wants());
  }

private:
  rtc::VideoSourceInterface<webrtc::VideoFrame> *source() override {
    return &this->broadcaster_;
  }
  rtc::scoped_refptr<webrtc::VideoCaptureModule> capturer_;
  std::unique_ptr<V4LDevice> device_;
  rtc::VideoBroadcaster broadcaster_;
};

WHIPSession::WHIPSession(std::string url) : url(url) {
  this->signaling_thread = rtc::Thread::CreateWithSocketServer();
  this->signaling_thread->Start();
}

void WHIPSession::Initialize() {
  this->factory = webrtc::CreatePeerConnectionFactory(
      nullptr, nullptr, this->signaling_thread.get(), nullptr,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);

  if (!this->factory) {
    tlog("Failed to create PeerConnectionFactory");
    return;
  }
}

bool WHIPSession::CreateConnection(bool dtls) {
  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = dtls;
  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = "stun:stun.l.google.com:19302";
  config.servers.push_back(server);

  webrtc::PeerConnectionDependencies pc_dependencies(this);
  this->pc =
      this->factory->CreatePeerConnection(config, std::move(pc_dependencies));
  return this->pc != nullptr;
}

void WHIPSession::AddCaptureDevice(uint8_t device_idx) {
  std::string device_path = "/dev/video" + std::to_string(device_idx);
  rtc::scoped_refptr<CapturerTrackSource> video_device =
      CapturerTrackSource::Create(device_path);
  if (!video_device)
    throw std::runtime_error("Failed to create video device");

  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
      this->factory->CreateVideoTrack("video_label", video_device));

  auto result_or_error = this->pc->AddTrack(video_track_, {"stream_id"});
  if (!result_or_error.ok()) {
    tlog("Failed to add video track: %s", result_or_error.error().message());
  }
}

void WHIPSession::CreateOffer() {
  tlog("Creating Offer: %d", this->pc->signaling_state());
  tlog("Senders: %d", this->pc->GetSenders().size());
  this->signaling_thread->PostTask(RTC_FROM_HERE, [this]() {
    this->pc->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  });
}

void WHIPSession::WaitForOffer() {
  while (this->sdp.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void WHIPSession::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>
        &streams) {
  tlog("OnAddTrack");
}

void WHIPSession::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  tlog("OnRemoveTrack");
}

void WHIPSession::OnIceCandidate(
    const webrtc::IceCandidateInterface *candidate) {
  tlog("OnIceCandidate %s %d", candidate->sdp_mid().c_str(),
       candidate->sdp_mline_index());
  std::string sdp;
  candidate->ToString(&sdp);
  tlog("Candidate: %s", sdp.c_str());
}

void WHIPSession::OnSuccess(webrtc::SessionDescriptionInterface *desc) {
  this->pc->SetLocalDescription(DummySetSessionDescriptionObserver::Create(),
                                desc);

  desc->ToString(&sdp);

  tlog("SDP Generated");
  http::Request request(this->url);
  const auto response =
      request.send("POST", this->sdp, {{"Content-Type", "application/sdp"}});

  tlog("SDP Sent");
  if (!response.status.Ok) {
    tlog("Failed to send SDP");
    return;
  }

  for (auto &header : response.headerFields)
    tlog("%s: %s", header.first.c_str(),
         header.second.c_str()); // response.headerFields

  std::string response_body{response.body.begin(), response.body.end()};
  tlog("Response Body: %s", response_body.c_str());
  absl::optional<webrtc::SdpType> sdp_type_opt =
      webrtc::SdpTypeFromString(response_body);
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> remote_desc =
      webrtc::CreateSessionDescription(*sdp_type_opt, response_body, &error);
  if (!remote_desc) {
    tlog("Failed to create remote description");
    return;
  }
  this->pc->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(),
                                 remote_desc.release());
  tlog("Remote Description Set");
}

void WHIPSession::OnFailure(webrtc::RTCError error) {
  throw std::runtime_error("OnFailure");
  tlog("OnFailure %s: %s", ToString(error.type()), error.message());
}

void WHIPSession::OnFailure(const std::string &error) {
  throw std::runtime_error("OnFailure");
  tlog("OnFailure %s", error.c_str());
}
