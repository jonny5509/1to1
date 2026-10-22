#pragma once

// Prefix for BLE names. Each ESP32 appends the last 3 bytes of its MAC address,
// so multiple gateways can be distinguished without building separate firmware.
#ifndef LVS_GATEWAY_PREFIX
#define LVS_GATEWAY_PREFIX "LVS-Gateway"
#endif
