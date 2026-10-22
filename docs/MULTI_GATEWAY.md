# Multi-gateway dashboard

## What this adds

- A browser dashboard at `web/dashboard.html` that can connect to multiple ESP32 gateways advertising the existing Lovense-compatible service.
- Per-gateway intensity and stop commands, a “stop all connected” action, connection state, and local pairing notes.
- A configurable BLE advertised name in `src/gateway_config.h`, so separately flashed gateways can be distinguished in the Bluetooth picker.

## Browser requirements

Use Chrome or Edge on HTTPS (or localhost). Open the hosted dashboard or serve the `web/` directory over HTTPS. Connect each gateway from the dashboard and grant Bluetooth permission when prompted. The dashboard and another controller app may compete for the same gateway connection; disconnect the other app if connection fails.

Pairing notes are stored in the browser's local storage. They are labels only; they are not written to the toy or ESP32.

## Flashing multiple gateways

1. Use one ESP32 board per Lovense-compatible gateway.
2. Before building each board, edit `LVS_GATEWAY_NAME` in `src/gateway_config.h` to a distinct name, for example `LVS-Gateway01` and `LVS-Gateway02`.
3. Build and flash each board separately using PlatformIO:
   ```sh
   pio run -e esp32s3
   pio run -e esp32s3 -t upload
   ```
   Select `esp32dev` instead if you use a classic ESP32 dev board.
4. Place each gateway near the toy you intend to test, connect them in the dashboard, and test one at a time at low intensity.

## Important protocol limitation

This does **not** yet provide guaranteed one-to-one control of multiple physical LoveSpouse/MuSE toys. The gateway controls the toys by advertising manufacturer-data packets; the packet does not contain a unique destination address. Any compatible toy in radio range may respond to a gateway's broadcast. A separate BLE identity distinguishes gateways to the control app, but does not identify a physical toy to the broadcast protocol.

The dashboard therefore offers independent commands **per gateway**, not verified independent commands **per physical toy**. Do not rely on the pairing note as an isolation guarantee. If multiple toys are within range, they may all react. True physical-device selection requires a verified device-specific protocol or tested RF isolation; neither is implemented or claimed here.

The firmware's reported battery/status values are placeholders in the original implementation. The dashboard does not present them as physical-toy telemetry.
