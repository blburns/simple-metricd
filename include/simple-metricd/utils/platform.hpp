/**
 * @file platform.hpp
 */

#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef SIMPLE_METRICD_WINDOWS
#define SIMPLE_METRICD_WINDOWS
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#if defined(__APPLE__)
#define SIMPLE_METRICD_MACOS
#elif defined(__FreeBSD__)
#define SIMPLE_METRICD_FREEBSD
#elif defined(__linux__)
#define SIMPLE_METRICD_LINUX
#else
#error "Unsupported platform"
#endif
#include <unistd.h>
#endif

namespace simple_metricd {

using port_t = uint16_t;

inline constexpr port_t kMetricsDefaultPort = 19100;

std::string platformName();

}  // namespace simple_metricd
