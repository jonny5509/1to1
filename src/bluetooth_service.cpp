#include <Arduino.h>
#include <esp_system.h>
#include "bluetooth_service.h"
#include "gateway_config.h"

static const char* TAG = "bluetooth_service";

void bluetooth_service_init() {
  // Derive a stable, distinct BLE name for every physical ESP32.
  // This avoids requiring a separate firmware build for each gateway.
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char deviceName[32];
  snprintf(deviceName, sizeof(deviceName), "%s-%02X%02X%02X",
           LVS_GATEWAY_PREFIX, mac[3], mac[4], mac[5]);

  NimBLEDevice::init(deviceName);
  NimBLEDevice::createServer();

  // NimBLE-Arduino 2.x API.
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->enableScanResponse(true);
}

void bluetooth_service_start() {
  NimBLEDevice::startAdvertising();
}

void bluetooth_service_stop() {
  NimBLEDevice::stopAdvertising();
}
