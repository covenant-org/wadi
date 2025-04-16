#include "logging.h"
#include "rtc_base/ssl_adapter.h"
#include "v4l.h"
#include "whip.h"
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  rtc::InitializeSSL();
  rtc::scoped_refptr<WHIPSession> session(
      new rtc::RefCountedObject<WHIPSession>(
          "http://159.54.131.60:8889/wadi/whip"));
  session->Initialize();
  if (session->CreateConnection(true)) {
    tlog("Connection created successfully");
  }
  session->AddCaptureDevice(0);
  session->CreateOffer();
  while(1);

  return 0;
}
