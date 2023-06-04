#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Arduino.h>
#include "images.h"
#include <Adafruit_SSD1306.h>

#include "config.h"
// include Wi-Fi library for esp32
#include <WiFi.h>
// mqtt
#include <PubSubClient.h>

#define SCK 5   // GPIO5  -- SX1278's SCK
#define MISO 19 // GPIO19 -- SX1278's MISO
#define MOSI 27 // GPIO27 -- SX1278's MOSI
#define SS 18   // GPIO18 -- SX1278's CS
#define RST 23  // GPIO14 -- SX1278's RESET
#define DI0 26  // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND 868E6

#define OLED_WEIGHT 128
#define OLED_HEIGHT 64
// oled i2c pins 21, 22

Adafruit_SSD1306 display(OLED_WEIGHT, OLED_HEIGHT, &Wire, -1);
String rssi;
String packSize;
String packet;

IPAddress ipFromStr(String ipStr)
{
    int *ip = new int[4];
    // ip.fromString(
    for (int i = 0; i < 4; i++)
    {
        ip[i] = ipStr.substring(0, ipStr.indexOf(".")).toInt();
        ipStr = ipStr.substring(ipStr.indexOf(".") + 1);
    }

    return IPAddress(ip[0], ip[1], ip[2], ip[3]);
}

// wifi client
WiFiClient espClient;
// mqtt
IPAddress serverMQTT = ipFromStr(MQTT_SERVER);
PubSubClient client(serverMQTT, 1883, espClient);

void init_lora()
{
    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DI0);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setSpreadingFactor(7);
    LoRa.setCodingRate4(5);
    LoRa.setPreambleLength(8);
    LoRa.setSyncWord(0x12);
    LoRa.setTxPower(17);

    if (!LoRa.begin(868E6))
    {
        Serial.println("Starting LoRa failed!");
        while (1)
            ;
    }
    Serial.println("LoRa Initializing OK!");
    // LoRa.onReceive(cbk);
    LoRa.receive();
    Serial.println("init ok");
}
void loraData()
{
    Serial.println("lora data");
    if (WiFiClass::status() != WL_CONNECTED)
    {
        Serial.println("Disconnected from wifi");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Disconnected from wifi");
        WiFi.setTxPower(WIFI_POWER_2dBm);
        // connect ot Wi-Fi
        WiFi.begin(mySSID, myPASS);
        // wait for connection
        Serial.println("Connecting ");
        while (WiFiClass::status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");
        Serial.println("Connected to the WiFi network");
    }

    if (!client.connected())
    {
        Serial.println("MQTT not connected");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("MQTT not connected");
        while (!client.connected())
        {
            delay(500);
            Serial.print(".");
            client.connect(MQTT_ID, MQTT_USER, MQTT_PASS);
        }
        Serial.println("");
        Serial.println("Connected to the MQTT network");
    }
    client.publish("test", packet.c_str());

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa Receiver");
    display.println("--------------");
    display.println("RSSI: " + rssi);
    display.println("Packet Size: " + packSize);
    display.println("Packet: " + packet);
    // display.println("BW: " + LoRa.);
    display.display();

    Serial.println("Received " + packSize + " bytes");
    Serial.println(packet);
    Serial.println(rssi);
}

void cbk(int packetSize)
{
    packet = "";
    packSize = String(packetSize, DEC);
    for (int i = 0; i < packetSize; i++)
    {
        packet += (char)LoRa.read();
    }
    rssi = String(LoRa.packetRssi(), DEC);
    loraData();
}
void init_oled()
{
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    delay(2000);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.cp437(true);
    display.println("LoRa Receiver");
    display.println("--------------");
    display.display();
}

void setup()
{
    // turn Wi-Fi on
    Serial.begin(115200);
    Serial.println("Turning on wifi");

    WiFi.setTxPower(WIFI_POWER_2dBm);
    // connect ot Wi-Fi
    WiFi.begin(mySSID, myPASS);
    // wait for connection
    Serial.println("Connecting ");
    while (WiFiClass::status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connected to the WiFi network");

    // print the SSID of the network you're attached to:
    Serial.println(WiFi.SSID());
    // print your Wi-Fi shield's IP address:
    IPAddress selfIP = WiFi.localIP();
    Serial.println(selfIP);
    // print the received signal strength:
    long wifiRssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(wifiRssi);

    Serial.println("Connecting to MQTT");
    client.setServer(serverMQTT, 1883);
    client.setKeepAlive(15);
    //    client.setCallback(callback);
    client.connect(MQTT_ID, MQTT_USER, MQTT_PASS);
    sleep(1);

    Serial.println("LoRa Receiver");
    Serial.println("--------------");

    init_oled();
    delay(1000);

    //    while (!Serial);
    Serial.println();
    //    Serial.println("LoRa Receiver Callback");
    init_lora();

    delay(15);
}

void loop()
{
    client.loop();
    if (!client.connected())
    {
        Serial.println("try to connect again in main loop");
        client.connect(MQTT_ID, MQTT_USER, MQTT_PASS);
    }

    int packetSize = LoRa.parsePacket();
    if (packetSize)
    {
        cbk(packetSize);
    }
    delay(10);
}
