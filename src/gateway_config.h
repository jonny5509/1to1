#pragma once

// Give each physical ESP32 gateway a unique BLE name when using multiple boards.
// Edit this value before building firmware for each board.
// The browser dashboard's pairing note is separate and is saved only in that browser.
#ifndef LVS_GATEWAY_NAME
#define LVS_GATEWAY_NAME "LVS-Gateway01"
#endif
