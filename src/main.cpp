#include "logging.h"
#include "v4l.h"
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  V4LDevice device("/dev/video0");

  struct v4l2_format fmt;
  if (device.get_format(&fmt) < 0) {
    tlog("Failed to get video format");
    return 1;
  }

  return 0;
}
