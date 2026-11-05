#include <argp.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

#include "brightness.h"

const char *argp_program_bug_address = "sujitbalasubramanianr@gmail.com";
const char *argp_program_version = "version 0.0.1";

int parse_arg_percent_to_brightness_value(const device &dev, const char *arg_str);

enum Operation
{
  OP_LIST,
  OP_GET,
  OP_SET
};

struct app_ctx
{
  char *device;
  char *cls;
  char *value;
  Operation operation;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  auto *ctx = static_cast<struct app_ctx *>(state->input);

  switch (key)
  {
  case 'l':
    ctx->operation = OP_LIST;
    break;
  case 'g':
    ctx->operation = OP_GET;
    break;
  case 's':
    ctx->operation = OP_SET;
    ctx->value = arg;
    break;
  case 'd':
    ctx->device = arg;
    break;
  case 'c':
    if (std::strcmp(arg, "backlight") != 0 && std::strcmp(arg, "leds") != 0)
    {
      argp_error(state, "Invalid class '%s'. Only 'backlight' or 'leds' are allowed.\n", arg);
    }
    ctx->cls = arg;
    break;
  case ARGP_KEY_INIT:
    ctx->cls = nullptr;
    ctx->device = nullptr;
    ctx->value = nullptr;
    ctx->operation = OP_LIST;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp_option options[] = {
    {"list", 'l', nullptr, 0, "List available devices with brightness control.", 0},
    {"get", 'g', nullptr, 0, "Get current brightness of the device.", 0},
    {"set", 's', "VALUE", 0, "Set brightness of the device (e.g. 50, +5, -10).", 0},
    {"device", 'd', "DEVICE", 0, "Specify device name.", 0},
    {"class", 'c', "CLASS", 0, "Specify class name ('backlight' or 'leds').", 0},
    {nullptr}};

static struct argp argp = {options, parse_opt, nullptr, nullptr, nullptr, nullptr, nullptr};

int main(int argc, char *argv[])
{
  struct app_ctx ctx = {};
  if (argp_parse(&argp, argc, argv, 0, nullptr, &ctx) != 0)
  {
    return EXIT_FAILURE;
  }

  std::vector<device> devs;
  int ret = get_all_device(devs, ctx.cls, ctx.device);
  if (ret < 0)
  {
    std::cerr << "Failed to fetch devices. Error: " << ret << std::endl;
    return EXIT_FAILURE;
  }

  if (devs.empty())
  {
    std::cerr << "No matching device found." << std::endl;
    return EXIT_FAILURE;
  }

  // Target the first matched device for get/set operations
  device &target_dev = devs[0];

  int val_to_be_updated = -1;
  if (ctx.value != nullptr)
  {
    val_to_be_updated = parse_arg_percent_to_brightness_value(target_dev, ctx.value);
  }

  switch (ctx.operation)
  {
  case OP_LIST:
    std::cout << std::left
              << std::setw(25) << "Device"
              << std::setw(15) << "Class"
              << "Brightness" << std::endl;

    for (const auto &d : devs)
    {
      std::cout << std::left
                << std::setw(25) << d.id
                << std::setw(15) << d.cls
                << std::right
                << std::setw(9) << get_brightness(d) << "%\n";
    }
    break;

  case OP_GET:
    std::cout << get_brightness(target_dev) << "%\n";
    break;

  case OP_SET:
    if (val_to_be_updated < 0)
    {
      std::cerr << "Invalid brightness value: " << (ctx.value ? ctx.value : "(null)") << '\n'
                << "Try 'brightness --help' for usage.\n";
      return EXIT_FAILURE;
    }

    if (set_brightness(target_dev, static_cast<unsigned int>(val_to_be_updated)) != 0)
    {
      std::cerr << "Failed to set brightness.\n";
      return EXIT_FAILURE;
    }

    std::cout << "Updated " << target_dev.id << " brightness to "
              << get_brightness(target_dev) << "%\n";
    break;

  default:
    std::cerr << "Try 'brightness --help' for usage.\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int parse_arg_percent_to_brightness_value(const device &dev, const char *arg_str)
{
  if (!arg_str)
    return -1;

  char *endptr = nullptr;
  errno = 0;

  long val = std::strtol(arg_str, &endptr, 10);

  if (arg_str == endptr || *endptr != '\0' || errno == ERANGE)
    return -1;

  if (*arg_str == '+' || *arg_str == '-')
    val = static_cast<long>(get_brightness(dev)) + val;

  if (val < 0)
    val = 0;
  if (val > 100)
    val = 100;

  return static_cast<int>((dev.max_brightness * val) / 100);
}