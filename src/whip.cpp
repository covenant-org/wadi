#include "whip.h"
#include <linux/videodev2.h>
#ifdef HW_ENCODING_SUPPORT
#include "encoder/jetson_encoder.h"
#endif
#include "HTTPRequest/Request.hpp"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/jsep.h"
#include "api/peer_connection_interface.h"
#include "api/rtc_error.h"
#include "api/rtp_parameters.h"
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
#include <algorithm>
#include <cstdint>
#include <sstream>
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
  if (fourcc == "YUY2" || fourcc == "YUYV") {
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

  static rtc::scoped_refptr<CapturerTrackSource>
  CreateWithConfig(std::string video_device_path, CaptureTrackConfig config) {
    std::unique_ptr<V4LDevice> device(new V4LDevice(video_device_path));
    tlog("Setting video capturer config %dx%d@%d", config.width, config.height,
         config.fps);
    device->fmt.fmt.pix.width = config.width;
    device->fmt.fmt.pix.height = config.height;
    device->fmt.fmt.pix.pixelformat = v4l2_fourcc(
        config.fourcc[0], config.fourcc[1], config.fourcc[2], config.fourcc[3]);
    device->framerate = config.fps;
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
//    assert(this->device_->sync_format());
    this->capturer_->RegisterCaptureDataCallback(&this->broadcaster_);

    struct webrtc::VideoCaptureCapability capability;
    capability.width = this->device_->fmt.fmt.pix.width;
    capability.height = this->device_->fmt.fmt.pix.height;
    capability.maxFPS = this->device_->framerate;
    capability.videoType = fourcc_to_videotype(
        fourcc_to_string(this->device_->fmt.fmt.pix.pixelformat));
    int res = this->capturer_->StartCapture(capability);
    if (res < 0) {
      tlog("Failed to start video capturer");
      assert(false);
    }
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
#ifdef HW_ENCODING_SUPPORT
      CreateJetsonEncoderFactory(),
#else
      webrtc::CreateBuiltinVideoEncoderFactory(),
#endif
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

void WHIPSession::AddCaptureDevice(uint8_t device_idx,
                                   std::optional<CaptureTrackConfig> config) {
  std::string device_path = "/dev/video" + std::to_string(device_idx);
  rtc::scoped_refptr<CapturerTrackSource> video_device =
      config.has_value()
          ? CapturerTrackSource::CreateWithConfig(device_path, config.value())
          : CapturerTrackSource::Create(device_path);
  if (!video_device)
    throw std::runtime_error("Failed to create video device");

  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
      this->factory->CreateVideoTrack("video_label", video_device));

  webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
      result_or_error = this->pc->AddTrack(video_track_, {"stream_id"});
  if (!result_or_error.ok()) {
    tlog("Failed to add video track: %s", result_or_error.error().message());
  }
  //  sender->SetParameters(params);
}

void WHIPSession::CreateOffer() {
  tlog("Creating Offer: %d", this->pc->signaling_state());
  tlog("Senders: %d", this->pc->GetSenders().size());
  this->signaling_thread->PostTask(RTC_FROM_HERE, [this]() {
    auto options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
    options.offer_to_receive_video = false;
    options.offer_to_receive_video = false;
    options.ice_restart = true;
    this->pc->CreateOffer(this, options);
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
  std::string sdp;
  candidate->ToString(&sdp);
  tlog("OnIceCandidate %s", sdp.c_str());
}

std::string
WHIPSession::SDPForceCodecs(const std::string sdp,
                            const std::vector<std::string> allowed_codecs) {

  std::istringstream isdpstream(sdp);
  std::ostringstream osdpstream;
  std::ostringstream rtmapstream;
  std::ostringstream midstream;
  std::string line;
  std::string current_mline;
  std::vector<std::string> allowed_ids;
  while (getline(isdpstream, line)) {
    if (line.find("m=video") != std::string::npos) {
      current_mline = line;
      continue;
    }
    if (line.find("a=rtpmap") != std::string::npos) {
      auto space = line.find_first_of(' ');
      std::string codec_id = line.substr(9, space - 9);
      auto slash = line.find_first_of('/');
      auto codec = line.substr(space + 1, slash - space - 1);
      if (std::find(allowed_codecs.begin(), allowed_codecs.end(), codec) !=
          allowed_codecs.end()) {
        allowed_ids.push_back(codec_id);
        rtmapstream << line << "\r\n";
      }
      continue;
    }
    if (line.find("a=rtcp-fb") != std::string::npos ||
        line.find("a=fmtp") != std::string::npos) {
      auto space = line.find_first_of(' ');
      auto dots = line.find_first_of(':');
      std::string codec_id = line.substr(dots + 1, space - dots - 1);
      if (std::find(allowed_ids.begin(), allowed_ids.end(), codec_id) !=
          allowed_ids.end()) {
        rtmapstream << line << "\r\n";
      }
      continue;
    }
    if (line.find("a=ssrc") != std::string::npos && !current_mline.empty()) {
      auto space = current_mline.find(" ");
      space = current_mline.find(" ", space + 1);
      space = current_mline.find(" ", space + 1);
      auto mline_no_codecs = current_mline.substr(0, space);
      osdpstream << mline_no_codecs;
      for (std::string &codec_id : allowed_ids) {
        osdpstream << " " << codec_id;
      }
      osdpstream << "\r\n";
      osdpstream << midstream.str();
      osdpstream << rtmapstream.str();
      midstream = std::ostringstream();
      rtmapstream = std::ostringstream();
      current_mline = std::string();
    }
    if (!current_mline.empty()) {
      midstream << line << "\r\n";
      continue;
    }
    osdpstream << line << "\r\n";
  }
  return osdpstream.str();
}

void WHIPSession::OnSuccess(webrtc::SessionDescriptionInterface *desc) {
  this->pc->SetLocalDescription(DummySetSessionDescriptionObserver::Create(),
                                desc);
  auto sender = this->pc->GetSenders()[0];
  webrtc::RtpParameters params = sender->GetParameters();
  tlog("Encodings %d", params.encodings.size());
  for (auto &encoding : params.encodings) {
    if (this->max_bitrate.has_value())
      encoding.max_bitrate_bps = this->max_bitrate.value();
    if (this->max_framerate.has_value())
      encoding.max_framerate = this->max_framerate;
  }
  sender->SetParameters(params);

  desc->ToString(&sdp);
  if (this->allowed_codecs.has_value())
    this->sdp = WHIPSession::SDPForceCodecs(sdp, this->allowed_codecs.value());
  tlog("SDP: %s", sdp.c_str());

  http::Request request(this->url);
  const auto response =
      request.send("POST", this->sdp, {{"Content-Type", "application/sdp"}});

  tlog("SDP Sent");
  if (!response.status.Ok) {
    tlog("Failed to send SDP");
    return;
  }

  std::string response_body{response.body.begin(), response.body.end()};
  tlog("SDP Response: %s", response_body.c_str());
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> remote_desc =
      webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, response_body,
                                       &error);
  if (!remote_desc) {
    tlog("Failed to create remote description");
    return;
  }
  this->pc->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(),
                                 remote_desc.release());

  return;
}

void WHIPSession::OnFailure(webrtc::RTCError error) {
  throw std::runtime_error("OnFailure");
  tlog("OnFailure %s: %s", ToString(error.type()), error.message());
}

void WHIPSession::OnFailure(const std::string &error) {
  throw std::runtime_error("OnFailure");
  tlog("OnFailure %s", error.c_str());
}
