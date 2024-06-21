#include <Wire.h>
#include <EEPROM.h>
#include <WebSocketsServer.h>

WebSocketsServer webSocket = WebSocketsServer(80);

const char* FIRMWARE_VERSION = "2.13.7"; // Define the firmware version

// rotation angle 53

//neccessary for void loop()
bool isRotationOn = true;//flag for rotation
bool dataToSend = true;
unsigned long previousMillisWifi = 0;
unsigned long previousMillisTime = 0;
long intervalWifi = 15000;//15seconds

//neccessary for dwin.begin()
const byte rxPin = 16; // rx2
const byte txPin = 17; // tx2
HardwareSerial dwin(1);

struct channelData {
  uint8_t channelID;
  uint8_t progressValue;
  uint32_t startTimeEpoch;
  uint32_t endTimeEpoch;
  long firstReading;
  char sampleName[17];
  uint16_t white1;
  uint16_t white2;
  uint16_t first1;
  uint16_t first2;
  uint16_t last1;
  uint16_t last2;
  bool isCompleted;
};

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  dwin.begin(115200, SERIAL_8N1, rxPin, txPin);

  initializeDisplay();

  RTCsetup();
  sensorSetup();
  spiffSetup();
  checkOTA ();
  //  check_device_parameters();
  wapSetup();
  gotoScreen(0);

  Serial.println("Device is on");
}

void loop()
{
  if (dwin.available()) fetchText(); // priority 1

  else {
    sendOldChannelDataToServer();

    controlTemp(); //priority 3
    processSerialCommand(); //priority 5

    readChannels();

    sendTemp(); // low priority

    if (isRotationOn) displayRotationScreen();

    long currentMillis = millis();
    if (currentMillis - previousMillisWifi >= intervalWifi) {
      checkWiFiConnection();
      previousMillisWifi = currentMillis;
    }

    if (currentMillis - previousMillisTime >= 60 * 1000) {
      updateDisplayWithCurrentTime();
      previousMillisTime = currentMillis;
    }
    webSocket.loop(); // Handle WebSocket events
  }
}
