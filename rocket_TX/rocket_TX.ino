#include <SPI.h>
#include <RH_RF95.h>

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
  // while(!Serial){}
  Serial1.begin(9600);

  delay(100);

  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  rf95.init();
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  rf95.setFrequency(RF95_FREQ);

  Serial.print("Set Freq to: ");Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

bool newGPSData;

struct GPSFix {
  long timestamp;
  float latitude;
  float longitude;
  float altitude;
  uint8_t fixQuality;
  uint8_t numSats;
  String toString(){
    return String(timestamp) + '\n' +
    String(latitude) + ", " +
    String(longitude) + ", " +
    String(altitude) + '\n' +
    "Fix Quality " + String(fixQuality) + '\n' +
    String(numSats) + " Satalites";
  }
};
GPSFix fix;

void loop()
{
  updateGPS();
  if(newGPSData){
    newGPSData = false;
    Serial.println();
    Serial.println(fix.toString());
    Serial.println();
    char* raw = (char*)&fix;
    rf95.send(raw, sizeof(GPSFix));
    rf95.waitPacketSent();
  }
  // rf95.send((uint8_t *)radiopacket, 20);
  // rf95.waitPacketSent();
  // if (rf95.waitAvailableTimeout(1000))
  //   if (rf95.recv(buf, &len))
}


String GPSBuffer = "";

void updateGPS(){
  while(Serial1.available()){
    char c = Serial1.read();
    // Serial.print("Buff: ");
    // Serial.println(GPSBuffer);
    if (c == '$') {
      // Process Command
      processCommand();
      // Clear Buffer
      GPSBuffer = "";
    }
    GPSBuffer += c;
  }
}

void processCommand(){
  // Only process GGA
  // http://aprs.gids.nl/nmea/#gga
  if(GPSBuffer.startsWith("$GPGGA")){
    unsigned int len = GPSBuffer.length();
    String command = GPSBuffer.substring(1, len - 5);
    String csstr = GPSBuffer.substring(len - 4, len - 2);

    Serial.println(command);
    int clen = command.length();
    byte cs1 = 0x00;
    for(int i = 0; i<clen; i++){
      cs1 ^= command[i];
    }
    byte cs2 = parseByte(csstr);
    bool cs = (cs1 == cs2);

    if(cs){
      // Parsing Command
      newGPSData = true;
      parseGGA(command);
    }else{
      Serial.print("    Warning: checksum incorrect: 0x");
      Serial.print(cs1, HEX);
      Serial.print(" 0x");
      Serial.print(cs2, HEX);
      Serial.print(" 0x");
      Serial.println(csstr);
    }
  }
}

void parseGGA(String command){

    // Find Commas
    int indecies[11];
    indecies[0] = 5;
    for(int i = 1; i<10; i++){
      indecies[i] = command.indexOf(',', indecies[i - 1] + 1);
    }



    // Extract data
    fix.timestamp = command.substring(indecies[0]+1, indecies[1]).toFloat();
  
    String _lat = command.substring(indecies[1]+1, indecies[2]);
    char _ns = command[indecies[2]+1];
    fix.latitude = _lat.toFloat() * ((_ns == 'N') ? 1.0 : -1.0);

    String _lon = command.substring(indecies[3]+1, indecies[4]);
    char _ew = command[indecies[4]+1];
    fix.longitude = _lon.toFloat() * ((_ew == 'E') ? 1.0 : -1.0);

    fix.fixQuality = command.substring(indecies[5]+1, indecies[6]).toInt();
    fix.numSats = command.substring(indecies[6]+1, indecies[7]).toInt();

    fix.altitude = command.substring(indecies[8]+1, indecies[9]).toFloat();

}

byte parseByte(String hexValue){
  byte tens = (hexValue[0] <= '9') ? hexValue[0] - '0' : hexValue[0] - '7';
  byte ones = (hexValue[1] <= '9') ? hexValue[1] - '0' : hexValue[1] - '7';
  return (16 * tens) + ones;
}
