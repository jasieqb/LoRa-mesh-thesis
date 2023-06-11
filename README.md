# LoRa mesh system by Jan Bylina

LoRa mesh system created for my bachelor thesis. It is based on my own protocol.

## Project structure

* `esp32_node` - ESP32 node firmware, written in C++ using Arduino framework and PlatformIO
* `pico_node` - Raspberry Pi Pico node firmware, written in MicroPython
* `influxdb_mqtt_connector` - InfluxDB MQTT connector, written in Python
* `relay` - ESP32 relay firmware, written in C++ using Arduino framework and PlatformIO

## Run

Each directory contains its own README.md file with instructions how to run the project.

## Hardware

* Raspberry Pi Pico with [LoRa hat](https://www.waveshare.com/pico-lora-sx1262-868m.htm) - 2x
* ESP32 ([LoRa32](https://www.lilygo.cc/products/lora3)) - 2x
    * Relay
    * Node
* DHT11 - 2x
