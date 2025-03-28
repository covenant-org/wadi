#include <linux/videodev2.h>
#include <string>

std::string fourcc_to_string(uint32_t pixelformat);
class V4LDevice {
public:
  explicit V4LDevice(std::string);
  ~V4LDevice();
  int _open();
  int get_format(v4l2_format *);

private:
  std::string sysfs_path;
  int fd;
};
