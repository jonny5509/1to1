# LVS-Gateway

ESP32 firmware that speaks Lovense BLE protocol to broadcast commands for MuSE/LoveSpouse toys.

# Motivation

You bought a cheap vibrating toy from China that claims App support but you find out you can not control it via XToys.app/buttplug.io/etc. and want to make that work.

# Installation & Usage

Prerequisites

- ESP32 or ESP32-S3 development board
- USB cable
- Google Chrome or Microsoft Edge browser

Installation

1. Go to https://ingeniouskink.github.io/LVS-Gateway/
2. Plug in your ESP32 board
3. Press the [connect] button and follow the instructions

Use

- Place the ESP32 near the toy and power it on
- Connect to bluetooth device named "LVS-Gateway" on your preferred control device
- 

# FAQ

Q: Does this support XToys.app ?

A: Yes, just connect a Lovense device via bluetooth.

---

Q: Does this support buttplug.io

A: Probably yes (untested).

---

Q: Do you recommend buying MuSE/Lovespouse devices ?

A: No, see [iostindex.com](https://iostindex.com) for an overview.

---

Q: But what if I want a cheap one ?

A: No, see this brand of cheap toys on [iostindex.com](https://iostindex.com/?filter0Brand=Folove) with way better BLE connectivity.

---

Q: Can I control more than one device at the same time ?

A: Yes, all MuSE/Lovespouse devices near the ESP32 will receive the same commands.

---

Q: How do I now if this is working ?

A: If it vibrates, it's working. There is no other way to tell because of the non-standard way the toys use bluetooth.


# Connectivity

```mermaid
---
title: "LVS-Gateway connectivity overview"
---
flowchart LR
	259657(("Buttplug.io")) <==>|"Lovense BLE"| 474684[["LVS-Gateway"]]
	linkStyle 0 stroke:#0000ff,fill:none
	866602(("XToys.app")) <==>|"Lovense BLE"| 474684
	linkStyle 1 stroke:#0101ff,fill:none
	474684 -.->|"BLE (broadcast only) 🛜"| 396938{"LoveSpouse\ncompatible\ntoy"}
	linkStyle 2 stroke:#009aff,fill:none
	474684 -.->|"BLE (broadcast only) 🛜"| 633666{"MuSE compatible\ntoy"}
	linkStyle 3 stroke:#009aff,fill:none
```


# Multi-gateway dashboard

A browser dashboard is available at [web/dashboard.html](web/dashboard.html). It can connect to multiple ESP32 gateways and send intensity/stop commands to each connected gateway independently. See [the multi-gateway guide](docs/MULTI_GATEWAY.md) for setup instructions.

**Important:** this is independent control per gateway, not guaranteed one-to-one control per physical toy. LoveSpouse/MuSE commands are broadcast without a unique destination address, so compatible toys in range may all respond. The dashboard clearly reports this limitation and does not claim toy telemetry that the protocol cannot provide.


## Local Wi-Fi dashboard

The firmware starts a Wi-Fi access point and serves a local control page. After flashing, open the serial monitor at 115200 baud, connect to the SSID and password printed by the device, then visit **http://192.168.4.1/**. The page works without internet and can send intensity/stop commands to that gateway. See [the setup guide](docs/MULTI_GATEWAY.md).

This is AP mode, not joining your home Wi-Fi. The local page uses HTTP and should remain on a trusted network. Current physical-toy commands are still broadcasts; nearby compatible toys may all respond.
