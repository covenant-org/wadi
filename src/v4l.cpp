#include "v4l.h"
#include "logging.h"
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
  memset(&fmt, 0, sizeof(fmt));
  memset(&cap, 0, sizeof(cap));
  this->_fill_cap();
  this->_fill_format();
  if (!this->can_capture()) {
    tlog("Cannot capture video");
    throw std::runtime_error("Failed to capture video");
  }
  tlog("Can capture video");

  if (!this->can_stream()) {
    throw std::runtime_error("Failed to stream video");
  }
  tlog("Can stream video");
}
V4LDevice::~V4LDevice() { close(fd); }

bool V4LDevice::can_capture() {
  return cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ||
         cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE;
}
bool V4LDevice::can_stream() { return cap.capabilities & V4L2_CAP_STREAMING; }

int V4LDevice::_open() {
  tlog("Attempting to open video capture device");
  std::string device = this->sysfs_path;
  fd = open(device.c_str(), O_RDWR);
  if (fd < 0) {
    tlog("Failed to open video capture device");
    return -1;
  }
  tlog("Successfully opened video capture device");

  return 0;
}

bool V4LDevice::sync_format() {
  v4l2_format desired_format = fmt;
  int ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
  if (ret < 0) {
    tlog("Failed to set video format");
    return false;
  }
  this->_fill_format();
  return fmt.fmt.pix.width == desired_format.fmt.pix.width &&
         fmt.fmt.pix.height == desired_format.fmt.pix.height &&
         fmt.fmt.pix.pixelformat == desired_format.fmt.pix.pixelformat;
}

int V4LDevice::_fill_cap() {
  tlog("Getting video capabilities");
  int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
  if (ret < 0) {
    tlog("Failed to get video capabilities");
    return -1;
  }
  tlog("Driver: %s", cap.driver);
  tlog("Card: %s", cap.card);
  tlog("Bus info: %s", cap.bus_info);
  return 0;
}

int V4LDevice::_fill_format() {
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
  if (ret < 0) {
    tlog("Failed to get video format");
    return 1;
  }
  std::string format = fourcc_to_string(fmt.fmt.pix.pixelformat);
  tlog("Pixel format: %s", format.c_str());
  tlog("Width: %d", fmt.fmt.pix.width);
  tlog("Height: %d", fmt.fmt.pix.height);
  return 0;
}
