#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include <string>
#include <vector>

typedef struct
{
  const char *cls;
  const std::string id;
  unsigned int curr_brightness;
  unsigned int max_brightness;
} device;

int get_all_device(std::vector<device> &devices, const char *class_filter, const char *device_filter);

unsigned int get_brightness(const device &device);

int set_brightness(device &device, unsigned int value);

#endif // !BRIGHTNESS_H
