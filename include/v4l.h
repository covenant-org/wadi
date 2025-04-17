#include <linux/videodev2.h>
#include <string>

std::string fourcc_to_string(uint32_t pixelformat);
class V4LDevice {
public:
  explicit V4LDevice(std::string);
  ~V4LDevice();
  v4l2_capability cap;
  v4l2_format fmt;
  uint32_t framerate;
  bool can_capture();
  bool can_stream();
  bool sync_format();

private:
  int _open();
  int _list_formats();
  int _fill_format();
  int _fill_cap();
  std::string sysfs_path;
  int fd;
};
