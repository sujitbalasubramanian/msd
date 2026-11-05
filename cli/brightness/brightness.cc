#include "brightness.h"

#include <systemd/sd-bus.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

namespace fs = std::filesystem;

#define ROOT "/sys/class"

static std::optional<unsigned int> read_uint_file(const fs::path &file_path)
{
  std::ifstream file(file_path);
  unsigned int value = 0;
  if (file && (file >> value))
  {
    return value;
  }
  return std::nullopt;
}

int get_all_device(std::vector<device> &devices,
                   const char *class_filter,
                   const char *device_filter)
{
  for (const char *cls : {"backlight", "leds"})
  {
    // Skip if filter is set and does not match cls
    if (class_filter != nullptr && strcmp(class_filter, cls) != 0)
      continue;

    fs::path dir = fs::path(ROOT) / cls;
    std::error_code ec;

    if (!fs::is_directory(dir, ec) || ec)
      continue;

    auto it = fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec)
      continue;

    for (const auto &entry : it)
    {
      ec.clear();
      if (!entry.is_directory(ec) || ec)
        continue;

      device dev{
          .cls = cls,
          .id = entry.path().filename().string(),
          .curr_brightness = 0,
          .max_brightness = 0,
      };

      // Skip if filter is set and does not match dev.id
      if (device_filter != nullptr && strcmp(device_filter, dev.id.c_str()) != 0)
        continue;

      if (auto curr = read_uint_file(dir / dev.id / "brightness"))
        dev.curr_brightness = *curr;
      else
        continue;

      if (auto max = read_uint_file(dir / dev.id / "max_brightness"))
        dev.max_brightness = *max;
      else
        continue;

      devices.push_back(std::move(dev));
    }
  }

  return devices.size();
}

int set_brightness(device &dev, unsigned int value)
{
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus *bus = nullptr;
  int r;

  r = sd_bus_open_system(&bus);
  if (r < 0)
  {
    std::cerr << "Failed to connect to system bus: " << strerror(-r) << std::endl;
    return r;
  }

  r = sd_bus_call_method(bus, "org.freedesktop.login1",
                         "/org/freedesktop/login1/session/auto",
                         "org.freedesktop.login1.Session", "SetBrightness",
                         &error, nullptr, "ssu", dev.cls, dev.id.c_str(), value);

  if (r < 0)
  {
    std::cerr << "Failed to issue method call: " << (error.message ? error.message : strerror(-r)) << std::endl;
  }

  sd_bus_error_free(&error);
  sd_bus_unref(bus);

  if (r < 0)
    return -1;

  dev.curr_brightness = value;
  return 0;
}

unsigned int get_brightness(const device &dev)
{
  if (dev.max_brightness == 0)
    return 0;

  return (dev.curr_brightness * 100) / dev.max_brightness;
}