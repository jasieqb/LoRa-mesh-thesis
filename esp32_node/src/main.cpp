#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESPRandom.h>
// #include <vector>

#define SCK 5   // GPIO5  -- SX1278's SCK
#define MISO 19 // GPIO19 -- SX1278's MISO
#define MOSI 27 // GPIO27 -- SX1278's MOSI
#define SS 18   // GPIO18 -- SX1278's CS
#define RST 23  // GPIO14 -- SX1278's RESET
#define DI0 26  // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND 868E6

#define OLED_WEIGHT 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C


String GLOBAL_ID = "ID" + String(ESP.getEfuseMac(), HEX);

unsigned long oledMillis = millis();
unsigned long measurmentMillis = millis();
unsigned long resendMillis;

const unsigned long intervalOled = 100UL;
const unsigned long intervalMeasurment = 300000UL;
const unsigned long intervalResend = 100UL;

String rssi = "";
String packSize = "";
String packet = "";

Adafruit_SSD1306 display(OLED_WEIGHT, OLED_HEIGHT, &Wire, -1);

bool isPacketAvailable = false;

void loraDataToOled()
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa SENDER");
    display.println("--------------");
    display.println("RSSI: " + rssi);
    display.println("Packet Size: " + packSize);
    display.println("Packet: " + packet);
    display.display();
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

    isPacketAvailable = true;
}

void init_lora()
{
    Serial.println("LoRa Initializing ...");

    // Initialize SPI
    SPI.begin(SCK, MISO, MOSI, SS);

    // Initialize LoRa
    LoRa.setPins(SS, RST, DI0);

    // Set Lora properties
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setSpreadingFactor(7);
    LoRa.setCodingRate4(5);
    LoRa.setPreambleLength(8);
    LoRa.setSyncWord(0x12);

    // Begin Lora communication
    if (!LoRa.begin(868E6))
    {
        Serial.println("Starting LoRa failed!");
        while (1)
            ;
    }
    Serial.println("LoRa Initializing OK!");

    // Set Lora callback
    LoRa.onReceive(&cbk);

    // Start listening for LoRa packets
    LoRa.receive();

    Serial.println("!!! LoRa initialized and listening !!!");
}

void init_oled()
{

    // Initialize OLED
    Serial.println("OLED Initializing ...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    Serial.println("OLED Initializing OK!");

    // Clear the buffer
    display.clearDisplay();

    // Display static text to test the display
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.cp437(true);
    display.println("LoRa SENDER");
    display.println("--------------");
    display.println("--------------");
    display.println("--------------");
    display.display();

    Serial.println("!!! OLED initialized !!!");

    delay(20);
}

String compose_json_message()
{
    uint8_t uuid[16];
    ESPRandom::uuid(uuid);
    String uuidStr = ESPRandom::uuidToString(uuid);

    DynamicJsonDocument doc(1024);
    doc["d_id"] = GLOBAL_ID;
    doc["m_id"] = uuidStr;
    doc["ttl"] = 10;
    // values as embedded document
    doc["values"] = JsonObject();
    doc["values"]["random_value"] = ESPRandom::get();
    doc["values"]["random_value2"] = ESPRandom::get();
    String mes = "";
    serializeJson(doc, mes);
    return mes;
}

void send_packet()
{
    String mes = compose_json_message();

    Serial.print("Send packet: ");
    Serial.println(mes);

    LoRa.beginPacket();

    LoRa.print(mes);
    LoRa.endPacket();
    LoRa.receive();
}

void setup()
{
    // milis for each loop
    oledMillis = millis();
    measurmentMillis = millis();

    // Serial port for debugging purposes
    Serial.begin(115200);

    // Init LoRa
    init_lora();

    // Init OLED
    init_oled();

    Serial.println("Setup done!");

    send_packet();
}

bool check_packet(StaticJsonDocument<1000> document)
{
    if (document.containsKey("d_id") && document.containsKey("ttl") && document.containsKey("m_id"))
    {
        return true;
    }
    return false;
}
bool isPacketToResend = false;
String to_resend = "";

void resend_message()
{
    LoRa.beginPacket();
    LoRa.print(to_resend);
    LoRa.endPacket();
    LoRa.receive();
    to_resend = "";

    Serial.println("Packet resent");
}

void handle_message()
{
    Serial.println("Handle message: " + packet);

    // Parse packet
    StaticJsonDocument<1000> doc;
    deserializeJson(doc, packet);

    if (!check_packet(doc))
    {
        Serial.println("Packet is not valid");
        to_resend = "";
        return;
    }

    // chcek if packet comes from this device
    if (doc["d_id"] == GLOBAL_ID)
    {
        Serial.println("Packet from myself");
        to_resend = "";
        return;
    }
    // chcek ttl
    if (doc["ttl"] == 0)
    {
        Serial.println("Packet ttl is 0");
        to_resend = "";
        return;
    }

    // decrease ttl
    int tmp_ttl = doc["ttl"];
    tmp_ttl--;
    doc["ttl"] = tmp_ttl;

    serializeJson(doc, to_resend);

    isPacketToResend = true;
    resendMillis = millis();
}

void loop()
{

    unsigned long currentMillis;

    currentMillis = millis();
    if (currentMillis - measurmentMillis >= intervalMeasurment)
    {
        measurmentMillis = currentMillis;
        send_packet();
    }

    if (currentMillis - oledMillis >= intervalOled)
    {
        oledMillis = currentMillis;
        loraDataToOled();
    }

    if (isPacketToResend and (currentMillis - resendMillis >= intervalResend))
    {
        resend_message();
        isPacketToResend = false;
        resendMillis = currentMillis;
    }
    if (isPacketAvailable)
    {
        handle_message();
        isPacketAvailable = false;
    }
}
