void initializeDisplay() {

  displayReset();
  gotoScreen(7);
  fetch_mac();

  readFromEEPROM(ssid, 66, 50);
  Serial.print("SSID: "); Serial.println(ssid);
  sendText(ssidAdd, ssid, 50);
  sendText(logBoxAdd, ssid, 100);

  sendText(firmwareAdd, FIRMWARE_VERSION, 7);

  readFromEEPROM(password, 116, 50);
  Serial.print("Password: "); Serial.println(password);
  sendText(passAdd, password, 50);
  sendText(logBoxAdd, password, 100);

  connectToWiFi();
  delay(1000);

  serialNum = EEPROM.read(64) * 255 + EEPROM.read(65);
  deviceID = "MBS" + String(serialNum + 100000).substring(1);
  Serial.println(deviceID);
  sendText(deviceIDadd, deviceID.c_str(), 50);
  sendText(logBoxAdd, deviceID.c_str(), 100);

  sendText(helplineadd, macStr, 50);
  sendText(rotationMsg, "Delay Rotation(60sec)", 30);

  //     Initialize all channels from 1 to 16
  for (int i = 1; i <= 16; i++) {
    resetChannel(i);
  }

}

void connectToWiFi() {
  WiFi.begin(ssid, password);
}

void checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED && !isWifiConnected ) {
    Serial.println("Entered Wifi connected");
    hideData(SPwifiIcon); // Connected
    Serial.println("Wifi Connected!");
    isWifiConnected = true;
    syncRtcTime();
    dataToSend = true;
  }

  else if (WiFi.status() != WL_CONNECTED)
  {
    isWifiConnected = false;
    showData(SPwifiIcon, wifiIcon); // Disconnected
    connectToWiFi();
  }

  fetchText();

  if (WiFi.status() == WL_CONNECTED) {
    if (millis() / 60000 - previousSynctimeMin >= 30) { // 30min elapsed since last sync
      syncRtcTime();
    }
  }

}


void updateProgress(int channel) {
  sendNum(progressArray[channel - 1], progressValues[channel - 1]);
  //send data via WAP
  char data[50]; // Adjust the size according to your data format
  sprintf(data, "Progress %d: %d", channel, progressValues[channel - 1]);
  sendDataToDesktop(data);
}


void updateElapsedTime(int channel, uint16_t decolorTimeMin) {

  uint16_t elapsedTime;

  if (isRunning[channel - 1]) {
    elapsedTime = ( getCurrentEpoch() - channelStartTimes[channel - 1] ) / 60; // Convert seconds to minutes
  }

  else if (isCompleted[channel - 1]) {
    elapsedTime = decolorTimeMin;
  }

  mbrt[channel - 1] = elapsedTime;

  fetchText();

  //-------------------------------formatting-----------------------------
  // Format elapsed time as a string (e.g., "02:04")
  String elapsedTimeStr = String(elapsedTime / 60, DEC);
  elapsedTimeStr += ":";
  elapsedTimeStr += String(elapsedTime % 60, DEC);

  // Pad with leading zeros if needed
  if (elapsedTime / 60 < 10) {
    elapsedTimeStr = "0" + elapsedTimeStr;
  }
  if (elapsedTime % 60 < 10) {
    elapsedTimeStr = elapsedTimeStr.substring(0, 3) + "0" + elapsedTimeStr.substring(3);
  }

  // Check if the elapsed time has changed
  if (elapsedTimeStr != lastElapsedTime[channel - 1]) {
    sendText(timeArray[channel - 1], elapsedTimeStr.c_str(), 5);
    lastElapsedTime[channel - 1] = elapsedTimeStr;
    char data[50]; // Adjust the size according to your data format
    sprintf(data, "Elapsed time %d: %s", channel, elapsedTimeStr.c_str());
    sendDataToDesktop(data);
  }

}


void startResumeChannel(bool isResuming, uint8_t channelID_,
                        const char* sampleName_, long firstReading_,
                        uint32_t startTimeEpoch_) {

  if (channelID_ < 1 || channelID_ > 16) {
    return;
  }

  if (firstReading_ == -2147483648) { //either no testtube or already white testtube. dont start in this case.
    completedChannel(true, channelID_, 0);
    beep(500);
    return;
  }

  Serial.print("Started Channel "); Serial.print(channelID_);

  if (noChannelRunning()) {
    Serial.println("No channel running. updating Rotation Epoch.");
    previousRotationEpoch = getCurrentEpoch();
    savePreviousRotationEpoch();
  }

  // flag to be updated after calling the noChannelRunning()

  isRunning[channelID_ - 1] = true;
  isCompleted[channelID_ - 1] = false;

  //  dataToSend = true;
  //  startPos --;;

  if (!isResuming) latestTestCounts[channelID_ - 1] = testCount;
  Serial.print(" with test count: "); Serial.print(latestTestCounts[channelID_ - 1]);

  channelStartTimes[channelID_ - 1] = startTimeEpoch_ ;
  Serial.print(" at StartTime: "); Serial.print(channelStartTimes[channelID_ - 1]);

  firstReading[channelID_ - 1] = firstReading_;
  Serial.print(" with firstReading: "); Serial.println(firstReading[channelID_ - 1]);

  fetchText();

  // UI changes
  char channelIDText[3];
  sprintf(channelIDText, "%02d", channelID_);
  sendText(channelIdArray[channelID_ - 1], channelIDText, 2); // Set channel ID based on the argument
  sendNum(progressArray[channelID_ - 1], 0); // Initially progress = 0
  sendText(startArray[channelID_ - 1], "", 9); // Empty string for "Start" text
  showData(SPprogressArray[channelID_ - 1], progressArray[channelID_ - 1]);
  showData(SPprogressbar[channelID_ - 1], progressArray[channelID_ - 1]);
  changeColor(SPprogressbar[channelID_ - 1], 6, 0xE73C); // Change foreground
  changeColor(SPprogressbar[channelID_ - 1], 7, 0xBEDF); // Change background
  changeColor(SPprogressArray[channelID_ - 1], 3, 0x20E4); // Change Progress%
  changeColor(SPtimeArray[channelID_ - 1], 3, 0x20E4); // Change Time
  sendText(nameArray[channelID_ - 1], sampleName_, 16);

  fetchText();

  if (!isResuming) {
    saveStartChannel(channelID_, sampleName_ , startTimeEpoch_, firstReading[channelID_ - 1], whiteIntensity[2 * (channelID_ - 1)], whiteIntensity[2 * (channelID_ - 1) + 1], first1, first2);
    sendText(timeArray[channelID_ - 1], "00:00", 5);
    channelReadCount[channelID_ - 1] = 0; //so that it retakes the firstreading in readChannel()
  }
  else {
    //sendText(timeArray[channelID_ - 1], "Calc", 5); //chatgpt
    updateElapsedTime(channelID_, 5); //ignore 5,
    channelReadCount[channelID_ - 1] = 20; //so that it doesn't retake the firstreading in readChannel()
  }

}


void stopChannel(int i) {

  //  dataToSend = true;
  //  startPos --;;

  // Check if the channel ID is valid
  if (i < 1 || i > 16) {
    return;
  }

  if (isRunning[i - 1] == false && isCompleted[i - 1] == true) {
    resetChannel(i);
    isRunning[i - 1] = false;
    isCompleted[i - 1] = false;
    saveStopChannel(i, 10000, progressValues[i - 1], latestIntensity[2 * (i - 1)], latestIntensity[2 * (i - 1) + 1], false); //only last parameter 'false' is going to be update. rest all wont update.
    return;
  }

  Serial.print("Stopped channel ");
  Serial.println(i);

  fetchText();

  uint32_t stopTime = getCurrentEpoch();
  saveStopChannel(i, stopTime, progressValues[i - 1], latestIntensity[2 * (i - 1)], latestIntensity[2 * (i - 1) + 1], false);

  // Mark the channel as not completed and not running
  isRunning[i - 1] = false;
  isCompleted[i - 1] = false;

  if (channelReadCount[i + 1] == 1) gotoScreen(0); // in case stopfunction changes the closelid FLAG


  // Reset the channel
  resetChannel(i);
}

void completedChannel(bool isResuming, uint8_t channelID, uint16_t decolorTimeMin ) {
  //
  //  dataToSend = true;
  //  startPos --;;

  Serial.print("Completed Channel " + String(channelID) + ", " );

  if (!isResuming) {
    uint32_t stopTime = getCurrentEpoch(); //adding offset of 5 minutes
    saveStopChannel(channelID, stopTime, progressValues[channelID - 1], latestIntensity[2 * (channelID - 1)], latestIntensity[2 * (channelID - 1) + 1], true);
    beep(500); //give beep only when not resuming
  }

  // UI changes
  if (isResuming) {
    updateElapsedTime(channelID, decolorTimeMin);
  }

  sendText(startArray[channelID - 1], "", 9); // Empty string for "Start" text
  char channelIDText[3];
  sprintf(channelIDText, "%02d", channelID);
  sendText(channelIdArray[channelID - 1], channelIDText, 2); // Set channel ID based on the argument
  changeColor(SPprogressbar[channelID - 1], 6, 0x04e5); // Change foreground
  changeColor(SPprogressbar[channelID - 1], 7, 0x04e5); // Change background
  changeColor(SPprogressArray[channelID - 1], 3, 0xffff); // Change Progress%
  changeColor(SPtimeArray[channelID - 1], 3, 0xffff); // Change Time
  showData(SPprogressbar[channelID - 1], progressArray[channelID - 1]);
  hideData(SPprogressArray[channelID - 1]);

  fetchText();

  //  Serial.print("complete channel counter: ");
  //  Serial.println( latestTestCounts[channelID - 1]);

  //keep the flags in the last!
  isRunning[channelID - 1] = false;
  isCompleted[channelID - 1] = true;

  if (channelReadCount[channelID + 1] == 1) gotoScreen(0); // in case stopfunction changes the closelid FLAG

}

String stringConvertedTime(int minutes) {
  // Calculate hours and minutes
  int hours = minutes / 60;
  int remainingMinutes = minutes % 60;

  // Format as "HH:MM"
  char formattedTime[6];
  sprintf(formattedTime, "%02d:%02d", hours, remainingMinutes);

  return String(formattedTime);
}

void displayRotationScreen() {

  if (!noChannelRunning()) {

    uint32_t currentEpoch = getCurrentEpoch();

    if (currentEpoch == 0) {
      return;
    }

    if (timeExtension) rotationIntervalMinutes = 1;
    else rotationIntervalMinutes = rotationInterval;

    if (currentEpoch - previousRotationEpoch >= rotationIntervalMinutes * 60) {
      sendText(rotationMsg, "Delay Rotation(60sec)", 30);
      gotoScreen(6); // Rotation screen
      sendNum(countdownAdd, timerRotation);
      currentScreen = 6; // Update the current screen
    }

    // Execute the following part only when on the rotation screen (0x0006)
    if (currentScreen == 6) {
      // Countdown every 1 second
      if (currentEpoch - previousCountdownEpoch >= 1 && timerRotation > 0) {
        timerRotation--; // Decrement the countdown value every 1 second
        sendNum(countdownAdd, timerRotation); // Update the countdown value on the screen
        previousCountdownEpoch = currentEpoch; // Update the previousCountdownEpoch here
      }

      // If the countdown reaches 0, execute rotate function
      if (timerRotation == 0) {
        sendText(rotationMsg, "Rotating Now..", 30);
        rotate_();
        gotoScreen(0x00); // Home screen
        rotationIntervalMinutes = rotationInterval;
        timeExtension = false;
        timerRotation = 10; // Reset the countdown for the next interval
        //      previousCountdownEpoch = currentEpoch; // Reset the previousCountdownEpoch for the next countdown
        currentScreen = 0; // Update the current screen to home screen
      }
    }
  }
  else {
    //    gotoScreen(0x00);
  }
}


void saveToEEPROM(const char* data, int address, int maxLength) {
  for (int i = 0; i < maxLength; ++i) {
    char c = data[i];
    EEPROM.write(address + i, c);
    if (c == '\0') break; // Stop writing at null terminator
  }
  EEPROM.commit(); // Commit changes to EEPROM
}

void readFromEEPROM(char* buffer, int address, int maxLength) {
  for (int i = 0; i < maxLength; ++i) {
    char c = EEPROM.read(address + i);
    buffer[i] = c;
    if (c == '\0') break; // Stop reading at null terminator
  }
}

int readFrame() {
  // A frame must have atleast 3 bytes.
  if (dwin.available() < 3) {
    return 0;
  }
  if (dwin.read() != 0x5A) {
    return 0;
  }
  if (dwin.read() != 0xA5) {
    return 0;
  }

  int bytesToRead = dwin.read();
  // Test buffer overflow
  if ((bytesToRead + 3) > sizeof(incomingData)) {
    return 0;
  }

  int retry = 0;
  while ((dwin.available() < bytesToRead) and (retry < 10)) {
    delay(2);
    retry++;
  }

  if (retry >= 10) {
    return 0;
  }

  memset(incomingData, 0, sizeof(incomingData));
  incomingData[0] = 0x5A;
  incomingData[1] = 0xA5;
  incomingData[2] = (char)bytesToRead;
  for (int i = 0; i < bytesToRead; i++) {
    incomingData[i + 3] = (char)dwin.read();
  }
  return bytesToRead + 3;
}

void printFrame(int bytesRead) {
  Serial.print("Received data: ");
  for (int j = 0; j < bytesRead; j++) {
    Serial.print(incomingData[j], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


void fetchText() {
  int bytesRead = readFrame();
  if (bytesRead == 0) {
    return;
  }

  // ProcessFrame now
  if (incomingData[3] == (byte)0x83) {
    //    Serial.println("0x83 Received ");
    uint16_t buttonAddress = (incomingData[4] << 8) | incomingData[5];
    int k = 7;
    String receivedMessage = "";

    while (incomingData[k] != 0xFF) {
      receivedMessage += incomingData[k];
      k++;
    }

    // Check if the received address is from 0x5000 to 0x50F0
    if (buttonAddress >= 0x5000 && buttonAddress <= 0x50F0) {
      // Update selectedChannel with the channel number
      selectedChannel = (buttonAddress - 0x5000) / 0x10 + 1;

      //If a started channel is repressed, take it to Stop channel screen
      if (!isRunning[selectedChannel - 1] && !isCompleted[selectedChannel - 1]) {
        gotoScreen(0x0001);
        delay(10); //must delay to allow screen change
        touch(100, 75);
      }
      else if (isRunning[selectedChannel - 1] && !isCompleted[selectedChannel - 1]) {
        gotoScreen(0x0003); // 0003 is stop channel screen
      }
      else if (isCompleted[selectedChannel - 1]) {
        stopChannel(selectedChannel);
      }

      sendText(0x1980, "", 30); //reset white text box
      writeChannelNum();
    }
    else if (buttonAddress == 0x6000) {
      if (receivedMessage.isEmpty()) {
        receivedMessage = selectedChannel; // Set it to an empty string if it's empty
      }
      testCount++;
      startResumeChannel(false, selectedChannel, receivedMessage.c_str(), takeFirstReading(selectedChannel - 1), getCurrentEpoch());
    }
    else if (buttonAddress == ssidAdd) {
      receivedMessage.toCharArray(ssid, 50);
      Serial.print("Wifi SSID: "); Serial.println(ssid);
      saveToEEPROM(ssid, 66, 50); //66 is the memory start address, 50 is total length

      connectToWiFi();
    }
    else if (buttonAddress == passAdd) {
      receivedMessage.toCharArray(password, 50);
      Serial.print("Wifi Password: "); Serial.println(password);
      saveToEEPROM(password, 116, 50); //116 is the memory start address, 50 is total length

      connectToWiFi();
    }
    else if (buttonAddress == rotationIntervalAdd) {
      int intervalValue = receivedMessage.toInt(); // Convert String to int
      Serial.print("Rotation interval from display: ");
      Serial.println(intervalValue);
      writeRotationIntervalToEEPROM(intervalValue);
      setRotationIntervalFromEEPROM();
    }
    else if (buttonAddress == setTempAdd) {
      float newTemperature = receivedMessage.toFloat();
      Serial.print("SetTemp from display: ");
      Serial.println(newTemperature);
      writeNewTempToEEPROM(newTemperature);
    }
    else if (buttonAddress == 0x2020) {
      stopChannel(selectedChannel);
    }
    else if (buttonAddress == 0x1960) {
      //      takeWhite(selectedChannel - 1);
      setGainChannel(selectedChannel);
    }
    else if (buttonAddress == 0x1970) {
      for (int i = 0; i < numChannels; i++) {
        //        takeWhite(i);
        setGainChannel(i);
      }
    }
    else if (buttonAddress == 0x2310) {
      previousRotationEpoch = getCurrentEpoch();
      timeExtension = true; // make it 1
      timerRotation = 10;
      currentScreen = 0;
      Serial.println("extended rotation");
    }
    else if (buttonAddress == 0x2320) {
      checkWiFiConnection();
    }
    else if (buttonAddress == testRotate) {
      sendText(diagnosticBox, "Rotation Started", 100);
      rotate_();
      sendText(diagnosticBox, "Rotation Completed", 100);
    }
    else if (buttonAddress == testWhite) {
      sendText(diagnosticBox, "White Started", 100);
      setGainAll();
      sendText(diagnosticBox, "White Completed", 100);
      beep(500);

    }
    else if (buttonAddress == testBlue) {
      sendText(diagnosticBox, "Blue Started", 100);
      blueAll();
      sendText(diagnosticBox, "Blue Completed", 100);
    }

    // Empty the arrays to avoid garbage
    memset(incomingData, 0, sizeof(incomingData));
  }
}


void sendText(uint16_t vpAddress, const char* text, int textLength)
{
  eraseText(vpAddress, textLength); // Erase displayx based on the specified memory size

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(strlen(text) + 3); // Length: Write command(1), vp address (2);
  dwin.write(0x82); // Write command
  dwin.write(highByte(vpAddress)); // Write address
  dwin.write(lowByte(vpAddress)); // Write Address
  dwin.write(text, strlen(text));

}

void sendTemp() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisTemp >= intervalTemp) {
    previousMillisTemp = currentMillis;
    sendText(temperatureAdd, String(temperature, 1).c_str(), 4);

    char temperatureStr[10];
    dtostrf(temperature, 1, 2, temperatureStr); // Format temperature to 1 decimal place

    String message = String("temp:") + temperatureStr;
    sendDataToDesktop(message.c_str());
  }
  fetchText();
}



void sendNum(uint16_t vpAddress, int value)
{
  //  eraseText(vpAddress, textLength); // Erase display based on the specified memory size

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05); // Length: Write command(1), vp address (2), number(2);
  dwin.write(0x82); // Write command
  dwin.write(highByte(vpAddress)); // Write address
  dwin.write(lowByte(vpAddress)); // Write Address
  dwin.write(highByte(value)); // highbyte of number
  dwin.write(lowByte(value)); // lowbyte of number
}


void beep(uint16_t durationMs) {

  durationMs = durationMs / 8;

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05); // Length: Write command(1), vp address (2), number(2);
  dwin.write(0x82); // Write command

  dwin.write(0x00);
  dwin.write(0xA0);
  dwin.write(highByte(durationMs)); // Length: Write command(1), vp address (2), number(2);
  dwin.write(lowByte(durationMs)); // Write command
}

void eraseText(int vpAddress, int textLength)
{
  //clear text field on display - it an absolutely ugly solution but we can
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(textLength + 3); //Lengh 3: Write,address low and high
  dwin.write(0x82); //write command
  dwin.write(highByte(vpAddress));//write address
  dwin.write(lowByte(vpAddress)); //Write Address

  for (int i = 0; i < textLength; i++)
  {
    dwin.write((byte)0x20); //write message (0x20 is code for space)
  }
}

void hideData(uint16_t SPaddress) {
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05);
  dwin.write(0x82);
  dwin.write(highByte(SPaddress));
  dwin.write(lowByte(SPaddress));
  dwin.write(0xFF);
  dwin.write(0x00);
}

void showData(uint16_t SPaddress, uint16_t varaddress) {
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05);
  dwin.write(0x82);
  dwin.write(highByte(SPaddress));
  dwin.write(lowByte(SPaddress));
  dwin.write(highByte(varaddress));
  dwin.write(lowByte(varaddress));
}

void gotoScreen(uint16_t pageNumber) {
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x07);
  dwin.write(0x82);
  dwin.write(0x00);
  dwin.write(0x84);
  dwin.write(0x5A);
  dwin.write(0x01);
  dwin.write(highByte(pageNumber));
  dwin.write(lowByte(pageNumber));

}

void displayReset() {
  byte resetCommand[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x04, 0x55, 0xAA, 0x5A, 0xA5};
  dwin.write(resetCommand, sizeof(resetCommand));
  delay(750); // this delay ensures the ESP32 writes values to the display only after the display has fully booted up.
}


void touch(int x, int y) {
  // Convert X and Y values to hexadecimal
  byte x_high = (byte)(x >> 8); // High byte of X
  byte x_low = (byte)(x & 0xFF); // Low byte of X
  byte y_high = (byte)(y >> 8); // High byte of Y
  byte y_low = (byte)(y & 0xFF); // Low byte of Y

  // Define the data array
  byte data[] = {0x5A, 0xA5, 0x0B, 0x82, 0x00, 0xD4, 0x5A, 0xA5, 0x00, 0x04, x_high, x_low, y_high, y_low};
  // Calculate the length of the data array
  int dataLength = sizeof(data);

  // Send each byte of the data array
  for (int i = 0; i < dataLength; i++) {
    dwin.write(data[i]);
  }
}



void writeChannelNum() {
  sendText(0x2030, String(selectedChannel).c_str(), 2);
}

void changeColor(uint16_t mainAddress, int offset, uint16_t color) {
  uint16_t vpAddress = mainAddress + offset;

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05);
  dwin.write(0x82);
  dwin.write(highByte(vpAddress));
  dwin.write(lowByte(vpAddress));
  dwin.write(highByte(color));
  dwin.write(lowByte(color));
}

void resetChannel(int i) {
  // Reset elapsed time-related variables and arrays
  channelStartTimes[i - 1] = 0;
  lastElapsedTime[i - 1] = ""; // Assuming lastElapsedTime is a string array

  // Set initial values for display elements
  char startText[12];
  sprintf(startText, "Start  %02d", i);
  sendText(startArray[i - 1], startText, 9);
  sendText(channelIdArray[i - 1], "", 2); // Empty string for ChannelID
  sendText(nameArray[i - 1], "", 16); // Empty string for name
  sendText(timeArray[i - 1], "", 5); // Empty string for time
  sendNum(progressArray[i - 1], 0); // Initial progress value, adjust if needed
  hideData(SPprogressArray[i - 1]);
  hideData(SPprogressbar[i - 1]);
}

bool noChannelRunning() {
  for (int i = 0; i < 16; i++) {
    if (isRunning[i]) {
      return false;
    }
  }
  return true;
}
