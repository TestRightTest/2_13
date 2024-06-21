void wapSetup() {
  WiFi.softAP(wapssid, wappassword); // Start Access Point
  webSocket.begin(); // Start WebSocket server
  webSocket.onEvent(webSocketEvent); // Set event handler
  Serial.print("Started AP with: ");
  Serial.println(wapssid);
}

void sendInitialWAPData() {
  String jsonData;
  jsonData += "{";
  jsonData += "\"deviceDetails\":";
  jsonData += "\"" + String(rotationAngle) + "\",";
  jsonData += "\"" + String(getCurrentEpoch()) + "\",";
  jsonData += "\"" + String(macStr) + "\",";
  jsonData += "\"" + String(FIRMWARE_VERSION) + "\"";
  jsonData += "}";
  // Send the JSON data to the desktop
  //  Serial.print(jsonData);
  sendDataToDesktop(jsonData.c_str());
}

void DeviceNumber() {
  String jsonData;
  jsonData += "{";
  jsonData += "\"SerialNum\":";
  jsonData += "\"" + String(deviceID) + "\",";
  jsonData += "\"" + String(newTemp) + "\",";
  jsonData += "\"" + String(rotationInterval) + "\",";
//  jsonData += "\"" + String(macStr) + "\"";
//  jsonData += "\"" + String(FIRMWARE_VERSION) + "\"";
  jsonData += "}";
  // Send the JSON data to the desktop
  //  Serial.print(jsonData);
  sendDataToDesktop(jsonData.c_str());
}


void sendDataToDesktop(const char* data) {
  webSocket.broadcastTXT(data, strlen(data)); // Send message to all connected clients
}

//desktop test
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      sendDataToDesktop("Welcome to my ESP32 Access Point!");
      checkChannelsToResume();
      sendInitialWAPData();
      DeviceNumber();
      sendOldChannelDataToWap();
      Serial.print("data sent to WAP");
      break;
    case WStype_BIN:

      handleFirmwareUpdate(payload, length);

      break;
    case WStype_TEXT: {
        Serial.printf("[%u] Received text: %s\n", num, payload);
        // Convert payload to a string
        String payloadStr((char*)payload);
        // Check if payload starts with "start#"
        if (payloadStr.startsWith("start#")) {
          // Extract channel ID from payload
          int channelID = extractChannelIDFromDesktop(payloadStr);
          String sampleName = extractSampleNameFromDesktop(payloadStr);
          testCount++;
          startResumeChannel(false, channelID,  sampleName.c_str(), takeFirstReading(channelID - 1), getCurrentEpoch());
        }

        else if (payloadStr.startsWith("stop#")) {

          int hashIndex = payloadStr.indexOf('#');
          String channelStr = payloadStr.substring(hashIndex + 1);
          int channelID = channelStr.toInt();
          stopChannel(channelID);
        }

        else if (payloadStr.startsWith("white#")) {
          // Extract channel ID from payload
          Serial.println("stopping channel from test");

          int channelID = extractChannelIDFromDesktop(payloadStr);
          Serial.println("stopping channel from test");

          if (channelID >= 1 && channelID <= numChannels) {

            //            takeWhite(channelID - 1);
            setGainChannel(channelID);
          } else if (channelID == 100) {
            for (int i = 1; i <= numChannels; i++) {
              //              takeWhite(i);
            }
          }
        }
        else if (payloadStr.startsWith("raw")) {
          String channelStr = payloadStr.substring(3);
          int i = channelStr.toInt();
          Serial.print(String(i) + ": ");
          if (i >= 1 && i <= 16) {
            Serial.print(sensorRead(2 * (i - 1)) );
            Serial.print(". ");
            Serial.println(sensorRead(2 * (i - 1) + 1) );
          }
        } else if (payloadStr.startsWith("setgain")) {
          String channelStr = payloadStr.substring(7);
          int i = channelStr.toInt();
          Serial.println("setting gain for :" + String(i));
          setGainChannel(i);
        }
        else if (payloadStr == "setall") {
          setGainAll();
        }
        else if (payloadStr == "blueall") {
          blueAll();
        }
        else if (payloadStr.startsWith("blue")) {
          String channelStr = payloadStr.substring(4);
          int i = channelStr.toInt();
          Serial.println("checking blue for :" + String(i));
          blue(i);
        }
        else if (payloadStr == "temp") {
          Serial.print("Temperature: ");
          Serial.println(temperature, 1); //1 decimal
        } else if (payloadStr.startsWith("setserial")) {
          String serialStr = payloadStr.substring(9);
          int newSerialNum = serialStr.toInt();
          setSerialFromWAP(newSerialNum);
        } else if (payloadStr.startsWith("setrotation")) {
          // Extract the rotation interval value from the payload
          String intervalStr = payloadStr.substring(11);
          int newInterval = intervalStr.toInt();
          setRotationIntervalFromWAP(newInterval);

        } else if (payloadStr == "printwhite") {
          printwhite();

        } else if (payloadStr == "first") {
          for (int i = 0; i < numChannels; i++) {
            Serial.print(String(firstReading[i]) + ",");
          }
          Serial.println();
        } else if (payloadStr == "rotate") {
          rotate_();
        } else if (payloadStr == "readAll") {
          readSpiffData();
        } else if (payloadStr == "memory") {
          displaySpiffsUsage();
        } else if (payloadStr == "clear") {
          clearSpiffData();
        } else if (payloadStr == "synctime") {
          syncRtcTime();
        } else if (payloadStr.startsWith("settemp")) {
          String tempStr = payloadStr.substring(7);
          float newTemp = tempStr.toFloat();
          setTemperatureFromWAP(newTemp);
        }
        else if (payloadStr == "restart") {
          ESP.restart(); // Restart the ESP32 board
        }
        else if (payloadStr == "format") {
          SPIFFS.format();
        }
        else if (payloadStr == "time") {
          Serial.println(getCurrentEpoch());
        }
        else if (payloadStr == "testsensor") {
          testSensor();
        }

        else if (payloadStr == "adjusttemp") {
          settempDiff();
        }
        else if (payloadStr.startsWith("angle")) {
          String angleStr = payloadStr.substring(5);
          int angleValue = angleStr.toInt();
          setAngleFromWAP(angleValue);
        }

        else if (payloadStr == "time") {
          Serial.println(getCurrentEpoch());
        }
        else if (payloadStr == "mac") {
          fetch_mac();
        }
        else if (payloadStr == "logs") {
          logflag = true;
        }

        else if (payloadStr == "limit") {
          if (digitalRead(LIMIT_PIN) == LOW) Serial.println("Limit Switch Pressed");
          else Serial.println("Limit Switch NOT Pressed");
        }
        else {
          // Parse the payload and assign values if it contains "setdata" key
          DynamicJsonDocument doc(200);
          DeserializationError error = deserializeJson(doc, payloadStr);
          if (!error && doc.containsKey("setdata")) {
            parsePayloadAndAssignValues(payload, length);
          } else {
            Serial.println("Invalid command: " + payloadStr);
          }
        }
        webSocket.broadcastTXT(payload, length);
        break;
      }

    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
  }
}

int extractChannelIDFromDesktop(String command) {
  // Extract the channel ID from the "start#X", "stop#X", or "white#X" command
  if (command.startsWith("start#") || command.startsWith("stop#") || command.startsWith("white#")) {
    return command.substring(6).toInt();
  }
  return -1; // Return -1 if the command doesn't contain a valid channel ID
}

String extractSampleNameFromDesktop(String command) {
  int commaIndex = command.indexOf(',');
  if (commaIndex != -1) {
    return command.substring(commaIndex + 1);
  }
  return ""; // Return empty string if no sample name is found
}


void setRotationIntervalFromWAP(int newInterval) {
  if (newInterval >= minRotationInterval && newInterval <= maxRotationInterval) {
    writeRotationIntervalToEEPROM(newInterval);
    rotationInterval = newInterval;
    Serial.println("Rotation interval updated and saved to EEPROM: " + String(rotationInterval));
  } else {
    // Invalid rotation interval
    Serial.println("Invalid input. Rotation interval must be between 15 and 300 minutes.");
  }
}


void handleFirmwareUpdate(uint8_t* payload, size_t length) {
  static const size_t chunkSize = 12288; // Ensure chunkSize matches in both JS and ESP32
  if (!isUpdating) {
    Serial.println("Firmware binary received, starting update...");
    gotoScreen(7);
    sendText(logBoxAdd, "updating firmware..", 100);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Update begin failed");
      Update.printError(Serial);
      return;
    }
    isUpdating = true;
  }

  // Write received binary data to the update process
  size_t written = Update.write(payload, length);
  if (written != length) {
    Serial.println("Update write failed");
    Update.printError(Serial);
    return;
  } else {
    //    Serial.printf("Written %d bytes\n", written);
  }

  // Check for errors during the write process
  if (Update.hasError()) {
    Serial.println("Update has encountered an error");
    Update.printError(Serial);
    gotoScreen(0);

    return;
  }

  // Finalize the update if this is the last chunk
  if (length < chunkSize) {
    if (Update.end(true)) {
      Serial.println("Update success, rebooting...");
      isUpdating = false;
      ESP.restart();
    } else {
      Serial.println("Update not finished");
      Update.printError(Serial);
      isUpdating = false;
      gotoScreen(0);

    }
  }
}

void setAngleFromWAP(int angleValue ) {
  rotationAngle = angleValue;
  Serial.println("Rotation angle set to " + String(angleValue) + ".");
  // Save the rotationAngle value in EEPROM at address 301
  EEPROM.put(301, rotationAngle);
  EEPROM.commit(); // Save the changes to EEPROM
}

void setSerialFromWAP(int newSerialNum) {
  // Extract quotient and remainder
  byte quotient = newSerialNum / 255;
  byte remainder = newSerialNum % 255;

  // Save quotient and remainder to EEPROM
  EEPROM.write(64, quotient);
  EEPROM.write(65, remainder);
  EEPROM.commit();

  Serial.println("Serial value updated and saved to EEPROM.");

  deviceID = "MBS" + String(newSerialNum + 100000).substring(1);
  Serial.println(deviceID);
}

void setTemperatureFromWAP(float newTemp) {
  Serial.println("Entered temperature value: " + String(newTemp) + " °C");

  // Validate the input
  if (newTemp >= 30.0 && newTemp <= 50.0) { // Check if the input is within the range (30.0 to 50.0°C)
    writeNewTempToEEPROM(newTemp);
    Serial.println("Temperature value updated and saved to EEPROM.");
  } else {
    Serial.println("Invalid input. Temperature value must be between 30.0 and 50.0°C.");
  }
}


void parsePayloadAndAssignValues(uint8_t * payload, size_t length) {
  String payloadStr((char*)payload); // Convert payload to a string
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payloadStr);
  // Check for parsing errors
  if (error) {
    Serial.print("Parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  // Check if the JSON object contains the required fields
  if (doc.containsKey("temperature") && doc.containsKey("rotationAngle") &&
      doc.containsKey("rotationInterval") && doc.containsKey("serial") && doc.containsKey("currentTimeEpoch")) {
    // Assign values to variables
    float TempFromWAP = doc["temperature"].as<float>();
    int rotationAngleFromWAP = doc["rotationAngle"].as<int>();
    int newIntervalFromWAP = doc["rotationInterval"].as<int>();
    int newSerialNumFromWAP = doc["serial"].as<int>();
    uint32_t EpochFromDesktop = doc["currentTimeEpoch"].as<int>();

    // Set RTC time based on NTP time
    rtc.adjust(DateTime(EpochFromDesktop));
    // Print current RTC time after adjustment
    Serial.print("Adjusted RTC time: ");
    Serial.println(rtc.now().unixtime());
    setRotationIntervalFromWAP(newIntervalFromWAP);
    setAngleFromWAP(rotationAngleFromWAP);
    setTemperatureFromWAP(TempFromWAP);
    setSerialFromWAP(newSerialNumFromWAP);
    clearSpiffData();
    //    ESP.restart();
  } else {
    Serial.println("Payload is missing required fields.");
  }
}






void sendOldChannelDataToWap () {
    if ( dataToSend == false || getOldTestCountData() == 0 ) {
            Serial.println("No data to send");
      return;
    }

    HTTPClient http;

    int ServerID = pow(serialNum, 3) + 937;
    // Serial.println("SerialNum = " + String(serialNum) + "ServerID = " + String(ServerID) );

    String encodedSampleName = encodeSampleName(old_channelData.sampleName);

    // Construct the server path with the data
    String serverPath = serverName + oldLog +
                        "?p1=" + String(macStr) +
                        "&p2=" + String(ServerID) +
                        "&p3=" + String(encodedSampleName) +
                        "&p4=" + String(old_channelData.startTimeEpoch) +
                        "&p5=" + String(old_channelData.channelID) +
                        "&p6=" + old_testCountID +
                        "&p7=" + String(old_channelData.progressValue) +
                        "&p8=" + String(old_channelData.endTimeEpoch) +
                        "&p9=" + String(old_channelData.white1) + "," + String(old_channelData.white2) + "," + String(old_channelData.first1) + "," + String(old_channelData.first2) + "," + String(old_channelData.last1) + "," + String(old_channelData.last2);
//    Serial.println("Server Path: " + serverPath);

//    Serial.println("file name" + old_fileName);
  // send to desktop wap
  String dataToSend = "oldData: " + old_fileName + " " + serverPath;
  sendDataToDesktop(dataToSend.c_str());
}
