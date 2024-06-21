void sendStartChannelDataToServer (const channelData& data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    int ServerID = pow(serialNum, 3) + 937;
    // Serial.println("SerialNum = " + String(serialNum) + "ServerID = " + String(ServerID) );

    String encodedSampleName = encodeSampleName(data.sampleName);

    // Construct the server path with the data
    String serverPath = serverName + startLog +
                        "?p1=" + String(macStr) +
                        "&p2=" + String(ServerID) +
                        "&p3=" + String(encodedSampleName) +
                        "&p4=" + String(data.startTimeEpoch) +
                        "&p5=" + String(data.channelID) +
                        "&p6=" + String(latestTestCounts[data.channelID - 1]) +
                        "&p7=1:" + String(data.white1) + ",2:" + String(data.white2) + ",3:" + String(data.first1) + ",4:" + String(data.first2);
    //    Serial.println("Server Path: " + serverPath);

    // Send HTTP GET request
    if (http.begin(serverPath)) {
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        //        Serial.print("HTTP Response code: ");
        //        Serial.println(httpResponseCode);
        String payload = http.getString();
        //        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }
    else {
      Serial.println("Failed to connect to server.");
    }

  } else {
    Serial.println("WiFi Disconnected");
    return; // WiFi disconnected
  }
}
//
void sendEndChannelDataToServer (const channelData& data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    int ServerID = pow(serialNum, 3) + 937;
    // Serial.println("SerialNum = " + String(serialNum) + "ServerID = " + String(ServerID) );

    String encodedSampleName = encodeSampleName(data.sampleName);

    // Construct the server path with the data
    String serverPath = serverName + endLog +
                        "?p1=" + String(macStr) +
                        "&p2=" + String(ServerID) +
                        "&p3=" + String(latestTestCounts[data.channelID - 1]) +
                        "&p4=" + String(data.progressValue) +
                        "&p5=" + String(data.endTimeEpoch) +
                        "&p6=5:" + String(data.last1) + ",6:" + String(data.last2);
//    Serial.println("Server Path: " + serverPath);

    // Send HTTP GET request
    if (http.begin(serverPath)) {
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
//        Serial.print("HTTP Response code: ");
//        Serial.println(httpResponseCode);
        String payload = http.getString();
//        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }
    else {
      Serial.println("Failed to connect to server.");
    }

  } else {
    Serial.println("WiFi Disconnected");
    return; // WiFi disconnected
  }
}

void checkOTA ( ) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = serverName + checkOta +
                        "?p1=" + String(macStr) +
                        "&p2=" + FIRMWARE_VERSION;
    if (http.begin(serverPath)) {
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        if ( payload.startsWith("Y=") ) {
//          Serial.println("yes");
          sendText(logBoxAdd, "updating firmware..", 100);

          int index = payload.indexOf('=');
          String result = payload.substring(index + 1);
          Serial.println("Version Update: ");
          Serial.println(result);

          //update firmware
          WiFiClient client;
          httpUpdate.rebootOnUpdate(false); // remove automatic update
          t_httpUpdate_return ret = httpUpdate.update(client, serverIP, 80, fetchOta + "?p1=" + String(macStr) +  "&p2=" + FIRMWARE_VERSION );
          switch (ret) {
            case HTTP_UPDATE_FAILED:
              Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
              break;
            case HTTP_UPDATE_NO_UPDATES:
              Serial.println("HTTP_UPDATE_NO_UPDATES");
              break;
            case HTTP_UPDATE_OK:
              Serial.println("HTTP_UPDATE_OK");
              delay(1000); // Wait a second and restart
              ESP.restart();
              break;
          }
        }
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
    else {
      Serial.println("Failed to connect to server.");
    }
  } else {
    Serial.println("WiFi Disconnected");
    return; // WiFi disconnected
  }
}

void check_device_parameters() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String serverPath = serverName + fetchParameters +
                        "?p1=" + String(macStr);
    if (http.begin(serverPath)) {
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        //          Serial.println(payload);
        http.end();
      }
      else {
        Serial.println("Failed to connect to server.");
      }
    } else {
      Serial.println("WiFi Disconnected");
      return; // WiFi disconnected
    }
  }
}



String encodeSampleName(const char* sampleName) {
  String encodedName = "";
  for (int i = 0; i < strlen(sampleName); i++) {
    if (sampleName[i] == ' ') {
      encodedName += "%20";
    } else {
      encodedName += sampleName[i];
    }
  }
  //  Serial.println(encodedName);
  return encodedName;
}

void fetch_mac ( void ) {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  sprintf(wapssid, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.print("MAC Address: ");
  Serial.println(macStr);
}


int getOldTestCountData() {
  // Open SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    sendText(logBoxAdd, "Failed to mount SPIFFS", 100);
    return 0; // Return 0 if SPIFFS failed to mount
  }

  // Open directory
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  // Skip files until reaching the desired position
  for (int i = 0; i < startPos; i++) {
    if (!file) {
      // No more files in the directory
      root.close();
      startPos = 0;
      dataToSend = false;
      return 0; // Return 0 to indicate end of files
    }
    file = root.openNextFile();
  }

  // Iterate over files in SPIFFS
  while (file) {
    if (!file.isDirectory()) {
      old_fileName = file.name();
      // Check if the filename matches the pattern
      if (old_fileName.endsWith(".bin")) {
//        Serial.print(old_fileName);
        size_t bytesRead = file.read((uint8_t*)&old_channelData, sizeof(channelData));
        // Close the file
        file.close();
        int dotIndex = old_fileName.indexOf('.');
        int underscoreIndex = old_fileName.indexOf('_');
        old_testCountID = old_fileName.substring(0, underscoreIndex);
        String channelIDStr = old_fileName.substring(underscoreIndex + 1, dotIndex);
        if (bytesRead == sizeof(channelData) && underscoreIndex > 0) {
          // Close directory
          root.close();
          return 1;
        } else {
          Serial.println("Error: Failed to read data from file");
        }
      }
    }
    file = root.openNextFile();
  }

  // Close directory
  root.close();
  return 0;
}

void deleteOldTestCountData() {
  // Check if the file exists
  if (SPIFFS.exists("/" + old_fileName)) {
    // Delete the file
    if (SPIFFS.remove("/" + old_fileName)) {
      Serial.println("File deleted successfully");
    } else {
      Serial.println("Failed to delete file");
    }
  } else {
    Serial.println("File does not exist " + old_fileName);
  }
}

void sendOldChannelDataToServer () {
  if (WiFi.status() == WL_CONNECTED) {
    //get data to send


    if ( dataToSend == false || getOldTestCountData() == 0 ) {
      //      Serial.println("No data to send");
      return;
    }

    //    Serial.print("dataToSend: "); Serial.println(dataToSend);
    //    Serial.print("startPos: "); Serial.println(startPos);

    //    sendText(logBoxAdd, "Sending to Cloud", 100);
    //    gotoScreen(7);

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
                        "&p9=1:" + String(old_channelData.white1) + ",2:" + String(old_channelData.white2) + ",3:" + String(old_channelData.first1) + ",4:" + String(old_channelData.first2) + ",5:" + String(old_channelData.last1) + ",6:" + String(old_channelData.last2);
//    Serial.println("Server Path: " + serverPath);

    // Send HTTP GET request
    if (http.begin(serverPath)) {
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
//        Serial.print("Send : ");
//        Serial.println(httpResponseCode);
        String payload = http.getString();
        //        Serial.println(payload);
        if ( httpResponseCode == 200  && old_testCountID.toInt() < (latestTestCounts[old_channelData.channelID - 1] - 1)) {
          //delete the data
          deleteOldTestCountData();
        } else {
          startPos++;
          //DO NOTHING
        }
      }
      else {
        startPos++;
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }
    else {
      //      Serial.println("Failed to connect to server.");
    }

  } else {
    //    Serial.println("WiFi Disconnected");
    //    gotoScreen(0);
    dataToSend = true;
    startPos = 0;
    return; // WiFi disconnected
  }
  //  gotoScreen(0);
}
