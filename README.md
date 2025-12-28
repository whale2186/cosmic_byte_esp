# Cosmic Byte Blitz Dongle Auto-Connect (ESP8266 + LDR + Relays)

This project solves a common issue with certain wireless game controllers (such as **Cosmic Byte Blitz**) where the controller dongle must be connected **only after** the controller has fully powered on and entered the correct mode (XInput).

An **ESP8266** is used to:
- Monitor the **dongle LED** using an **LDR**
- Control **USB D+ and D− lines** using **two relays**
- Automatically connect the dongle to the PC at the correct time
- Provide a **web interface** for manual control

---

## 📁 Repository Structure

- `cosmic_byte_blitz_dongle_esp8266_client.ino`  
  → Main ESP8266 firmware

You can add:
- Circuit diagram image(s)
- Enclosure photos

---

## ⚙️ How It Works (Overview)

1. The controller is powered on first.
2. The dongle LED initially **blinks**.
3. An **LDR** detects LED instability (blinking).
4. While blinking → USB **D+ / D− are disconnected** via relays.
5. When the LED becomes **solid**, the ESP8266 detects stability.
6. The relays close, connecting **USB D+ and D−** to the PC.
7. The dongle enumerates correctly and the controller works reliably.

---

## 🧰 Hardware Required

- ESP8266 board (NodeMCU / ESP-12 / Wemos D1 mini)
- **2-channel relay module** (active-LOW preferred)
- **USB Type-A female adapter**
- **LDR (photoresistor)**
- 10kΩ resistor
- Jumper wires
- External 5V power supply

---

## 🔌 USB Female Adapter Wiring

USB Type-A female pinout:

| Pin | Signal |
|----|-------|
| 1 | +5V (VBUS) |
| 2 | D− |
| 3 | D+ |
| 4 | GND |

**Important:**
- Do **NOT** switch +5V or GND
- Only switch **D+ and D−**

### Relay routing
PC USB D+ → Relay 1 COM  
Relay 1 NO → Dongle D+  

PC USB D− → Relay 2 COM  
Relay 2 NO → Dongle D−  

---

## 🔁 Relay Wiring

| ESP8266 | Relay |
|--------|-------|
| D1 | IN1 (D+) |
| D2 | IN2 (D−) |
| GND | GND |
| VCC | VCC |

Relays are **active-LOW**.

---

## 🌞 LDR Wiring

Recommended voltage divider:

3.3V → LDR → A0 → 10kΩ → GND

Place the LDR close to the dongle LED and shield it from ambient light.

---

## 📶 WiFi Setup

Edit in the `.ino` file:

```cpp
#define SSID     "YOUR_WIFI_NAME"
#define PASSWORD "YOUR_WIFI_PASSWORD"
```

---

## 🌐 Web Interface

Open in browser:

```
http://<ESP8266-IP>
```

Controls:
- **LDR (Auto)** – automatic operation
- **Force ON** – always connect dongle
- **Force OFF** – always disconnect
- **Power OFF** – temporary disconnect then auto

---

## 🤖 Auto Mode Logic

- LED blinking → relays OFF
- LED solid → relays ON
- Uses stability timing and thresholds to avoid false triggers

---
