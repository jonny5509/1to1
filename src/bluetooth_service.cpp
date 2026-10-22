#include "bluetooth_service.h"
#include "gateway_config.h"

static const char* TAG = "bluetooth_service";

void bluetooth_service_init() {
  // Initialize the NimBLE device
  NimBLEDevice::init(LVS_GATEWAY_NAME);

  // Create a new server
  NimBLEDevice::createServer();

  // NimBLE-Arduino 2.x uses enableScanResponse(); the old
  // setScanResponse/setMinPreferred APIs were removed.
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->enableScanResponse(true);
}

void bluetooth_service_start() {
  NimBLEDevice::startAdvertising();
}

void bluetooth_service_stop() {
  NimBLEDevice::stopAdvertising();
}
