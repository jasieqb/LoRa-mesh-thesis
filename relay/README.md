# Relay

Firmware for the relay ESP32. It is written in C++ using Arduino framework and PlatformIO. It is used to connect the LoRa mesh network to the Internet. It sends data from the mesh network to the InfluxDB database using MQTT.

## Configuration

Open `include/config.h` and fill necessary variables.

## Run

1. Install [PlatformIO](https://platformio.org/platformio-ide)
2. Open project in PlatformIO
3. Build and upload firmware to ESP32
