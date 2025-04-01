#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include <string>

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
  static DummySetSessionDescriptionObserver *Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};

class WHIPSession : public webrtc::PeerConnectionObserver,
                    public webrtc::CreateSessionDescriptionObserver {
public:
  explicit WHIPSession(std::string url);
  ~WHIPSession(){};
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;
  std::string sdp;

  void Initialize();
  void AddCaptureDevice(uint8_t);
  bool CreateConnection(bool);
  void CreateOffer();
  void WaitForOffer();
  std::unique_ptr<rtc::Thread> signaling_thread;

  std::string url;
  void
  OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
             const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>
                 &streams) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
  void OnIceConnectionReceivingChange(bool receiving) override {}

  // CreateSessionDescriptionObserver implementation
  void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;
  void OnFailure(webrtc::RTCError error) override;
  void OnFailure(const std::string &error) override;
};
