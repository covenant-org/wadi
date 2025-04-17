#include "logging.h"
#include "rtc_base/ssl_adapter.h"
#include "v4l.h"
#include "whip.h"
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define BASE_VIDEO_PATH "/dev/video"

struct ParsedArgs {
  std::map<std::string, std::string> named;
  std::vector<std::string> positional;
};

ParsedArgs parse_args(int argc, char **argv) {
  ParsedArgs args;
  std::optional<std::string> key;
  for (int i = 1; i < argc; i += 1) {
    std::string token(argv[i]);
    if (key.has_value()) {
      args.named[key.value()] = token[0] == '-' ? "true" : token;
    }
    if (token[0] == '-') {
      key = token.substr(1);
      continue;
    }
    if (!key.has_value()) {
      args.positional.push_back(token);
    }
    key = std::nullopt;
  }
  return args;
}

class WadiConfig {
public:
  std::string whip_endpoint;
  std::string video_device;
  CaptureTrackConfig capture_config;

  WadiConfig(std::string whip_endpoint, std::string video_device,
             CaptureTrackConfig capture_config) {
    this->whip_endpoint = whip_endpoint;
    this->video_device = video_device;
    this->capture_config = capture_config;
  }

  WadiConfig() {
    this->whip_endpoint = "http://localhost:8889/wadi/whip";
    this->video_device = "0";
    this->capture_config.width = 1280;
    this->capture_config.height = 720;
    mempcpy(this->capture_config.fourcc, "I420",
            4); // this->capture_config.fourcc
    this->capture_config.fps = 30;
  }

  static WadiConfig FromArgs(int argc, char **argv) {
    ParsedArgs args = parse_args(argc, argv);
    WadiConfig config;
    if (args.positional.size() > 0) {
      config.whip_endpoint = args.positional[0];
    }
    if (args.named.find("d") != args.named.end()) {
      config.video_device = args.named["d"];
    }
    if (args.named.find("w") != args.named.end()) {
      config.capture_config.width = atoi(args.named["w"].c_str());
    }
    if (args.named.find("h") != args.named.end()) {
      config.capture_config.height = atoi(args.named["h"].c_str());
    }
    if (args.named.find("r") != args.named.end()) {
      config.capture_config.fps = atoi(args.named["r"].c_str());
    }
    if (args.named.find("c") != args.named.end()) {
      for (int i = 0; i < args.named["c"].length(); i++) {
        args.named["c"][i] = std::toupper(args.named["c"][i]);
      }
      mempcpy(config.capture_config.fourcc, args.named["c"].c_str(),
              args.named["c"].length() > 4 ? 4 : args.named["c"].length());
    }
    return config;
  }
};

int main(int argc, char **argv) {
  rtc::InitializeSSL();
  WadiConfig config = WadiConfig::FromArgs(argc, argv);
  rtc::scoped_refptr<WHIPSession> session(
      new rtc::RefCountedObject<WHIPSession>(config.whip_endpoint));
  tlog("Requesting connection to whip server %s", config.whip_endpoint.c_str());
  //"http://159.54.131.60:8889/wadi/whip"));
  session->Initialize();
  if (session->CreateConnection(true)) {
    tlog("Connection created successfully");
  }
  if (config.video_device.find(BASE_VIDEO_PATH) == 0) {
    config.video_device = config.video_device.substr(strlen(BASE_VIDEO_PATH));
  }
  session->AddCaptureDevice(atoi(config.video_device.c_str()),
                            config.capture_config);
  session->CreateOffer();
  while (1)
    ;

  return 0;
}
