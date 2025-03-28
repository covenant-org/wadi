#include "logging.h"
#include "v4l.h"
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

std::string fourcc_to_string(uint32_t pixelformat) {
  char str[4];
  str[0] = (pixelformat >> 0) & 0xFF;
  str[1] = (pixelformat >> 8) & 0xFF;
  str[2] = (pixelformat >> 16) & 0xFF;
  str[3] = (pixelformat >> 24) & 0xFF;
  return std::string(str, 4);
}

V4LDevice::V4LDevice(std::string path) : sysfs_path(path) {
  if (this->_open() < 0) {
    throw std::runtime_error("Failed to open video capture device");
  }
}
V4LDevice::~V4LDevice() { close(fd); }

int V4LDevice::_open() {
  tlog("Attempting to open video capture device");
  std::string device = this->sysfs_path;
  fd = open(device.c_str(), O_RDWR);
  if (fd < 0) {
    tlog("Failed to open video capture device");
    return -1;
  }
  tlog("Successfully opened video capture device");

  tlog("Getting video capabilities");
  struct v4l2_capability cap;
  std::memset(&cap, 0, sizeof(cap));
  int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
  if (ret < 0) {
    tlog("Failed to get video capabilities");
    return -1;
  }
  tlog("Driver: %s", cap.driver);
  tlog("Card: %s", cap.card);
  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ||
        cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
    tlog("Cannot capture video");
    return -1;
  }
  tlog("Can capture video");

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    tlog("Cannot stream video");
    return -1;
  }
  tlog("Can stream video");

  return 0;
}

int V4LDevice::get_format(v4l2_format *fmt) {
  fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int ret = ioctl(fd, VIDIOC_G_FMT, fmt);
  if (ret < 0) {
    tlog("Failed to get video format");
    return 1;
  }
  std::string format = fourcc_to_string(fmt->fmt.pix.pixelformat);
  tlog("Original pixel format: %s", format.c_str());
  tlog("Original Width: %d", fmt->fmt.pix.width);
  tlog("Original Height: %d", fmt->fmt.pix.height);
  return 0;
}
