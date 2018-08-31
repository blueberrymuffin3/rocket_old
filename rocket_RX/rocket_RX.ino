#include <SPI.h>
#include <RH_RF95.h>
#include <ArduinoJson.h>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  while(!Serial){}

  // Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  rf95.init();
  // Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  rf95.setFrequency(RF95_FREQ);

  // Serial.print("Set Freq to: ");Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

struct GPSFix {
  long timestamp;
  float latitude;
  float longitude;
  float altitude;
  uint8_t fixQuality;
  uint8_t numSats;
};
GPSFix fix;

void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now
    // uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(GPSFix);
    if (rf95.recv((char*)&fix, &len))
    {
      digitalWrite(LED_BUILTIN, HIGH);
      // RH_RF95::printBuffer("Received: ", (char*)&fix, len);
      // Serial.print("Got: ");
      // Serial.println((char*)buf);
      // Serial.println(fix.toString());
      // Serial.print("RSSI: ");
      // Serial.println(rf95.lastRssi(), DEC);
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& root = jsonBuffer.createObject();
      root["timestamp"] = fix.timestamp;
      root["latitude"] = fix.latitude;
      root["longitude"] = fix.longitude;
      root["altitude"] = fix.altitude;
      root["fixQuality"] = fix.fixQuality;
      root["numSats"] = fix.numSats;

      root.printTo(Serial);
      // Serial.println();

      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}

