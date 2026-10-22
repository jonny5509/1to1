#include <Arduino.h>
#include "lovense.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "muse.h"
#include "web_service.h"

static const char* TAG = "lovense";
#define LOVENSE_SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fb"
#define LOVENSE_CHARACTERISTIC_UUID "0000fff0-0000-1000-8000-00805f9b34fb"
static NimBLECharacteristic *pLovenseCharacteristic = nullptr;

class ConnectionEventHandler : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *pServer) { Serial.println("BLE client connected to Lovense-compatible service"); web_service_log("BLE client connected to ESP32 Lovense-compatible service"); }
    void onDisconnect(NimBLEServer *pServer) { Serial.println("BLE client disconnected from Lovense-compatible service"); web_service_log("BLE client disconnected from ESP32 Lovense-compatible service"); }
};

std::string generate_response(const std::string& command) {
    if (command == "DeviceType;") return get_device_info();
    if (command == "Battery;") return get_battery_level();
    if (command == "Status:1;") return get_device_status();
    if (command.find("AutoSwitch:") == 0) return set_auto_switch_options(command);
    if (command.find("Vibrate:") == 0) return set_vibration_speed(command);
    if (command == "PowerOff;") return set_vibration_speed(std::string("Vibrate:0;"));
    return "OK;";
}

std::string get_device_info() {
    std::string firmwareVersion = "01";
    std::string deviceType = "G";
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macAddressStr[18];
    sprintf(macAddressStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return deviceType + ":" + firmwareVersion + ":" + macAddressStr + ";";
}

std::string get_battery_level() { return "100;"; }
std::string get_device_status() { return "2;"; }
bool autoSwitchTurnOff = true;
bool autoSwitchLastLevel = true;

std::string get_auto_switch_options() {
    std::string response = "AutoSwitch:";
    response += autoSwitchTurnOff ? "1:" : "0:";
    response += autoSwitchLastLevel ? "1;" : "0;";
    return response;
}

std::string set_auto_switch_options(const std::string& command) {
    if (command == "AutoSwitch:On:Off;") { autoSwitchTurnOff = true; autoSwitchLastLevel = false; }
    else if (command == "AutoSwitch:On:On;") { autoSwitchTurnOff = true; autoSwitchLastLevel = true; }
    else if (command == "AutoSwitch:Off:On;") { autoSwitchTurnOff = false; autoSwitchLastLevel = true; }
    else if (command == "AutoSwitch:Off:Off;") { autoSwitchTurnOff = false; autoSwitchLastLevel = false; }
    return "OK;";
}

std::string set_vibration_speed(const std::string& command) {
    size_t colon = command.find(':');
    size_t end = command.find(';');
    if (colon == std::string::npos || end == std::string::npos || end <= colon + 1) return "Invalid command;";
    int speedInt = std::stoi(command.substr(colon + 1, end - colon - 1));
    if (speedInt < 0) speedInt = 0;
    if (speedInt > 20) speedInt = 20;
    float speed = static_cast<float>(speedInt) / 20.0f;
    Serial.print("Vibration Speed: ");
    Serial.println(speed);
    muse_set_intensity(speed);
    return "OK;";
}

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.empty()) return;
        Serial.print("Received Value: ");
        Serial.println(value.c_str());
        String event = String("BLE GATT write received: ") + String(value.c_str());
        web_service_log(event.c_str());
        std::string response = generate_response(value);
        Serial.print("Response Value: ");
        Serial.println(response.c_str());
        String responseEvent = String("BLE GATT response sent: ") + String(response.c_str());
        web_service_log(responseEvent.c_str());
        pCharacteristic->setValue(response);
        pCharacteristic->notify();
    }
};

void lovense_init() {
    NimBLEServer *pServer = NimBLEDevice::getServer();
    pServer->setCallbacks(new ConnectionEventHandler());
    NimBLEService *pService = pServer->createService(LOVENSE_SERVICE_UUID);
    pLovenseCharacteristic = pService->createCharacteristic(
        LOVENSE_CHARACTERISTIC_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    pLovenseCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pService->start();
    NimBLEDevice::getAdvertising()->addServiceUUID(LOVENSE_SERVICE_UUID);
}
