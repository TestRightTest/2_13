void RTCsetup() {

  // Initialize Wire instances
  I2Cone.begin(SDA_PHOTODIODE, SCL_PHOTODIODE, 100000);
  I2Ctwo.begin(SDA_TEMP, SCL_TEMP, 100000);

  if (rtc.begin(&I2Ctwo)) {
    //    Serial.println("RTC_DS3231 found using I2Ctwo");
  }
  else if (rtc.begin(&I2Cone)) {
    //    Serial.println("RTC_DS1307 found using I2Cone");
  }

  else {
    Serial.println("Couldn't find RTC");
    sendText(logBoxAdd, "Couldn't find RTC", 100);
    delay(1000);
    return;
  }

  while (!rtc.isrunning()) {
    Serial.println("RTC is not running!");
    sendText(logBoxAdd, "RTC is not running!", 100);
    delay(500);
  }

  timeClient.begin();
  delay(1000);
  syncRtcTime();
  updateDisplayWithCurrentTime();
}

uint32_t getCurrentEpoch() {
  uint32_t currentEpoch = rtc.now().unixtime();

  if (currentEpoch < 1714289503 || currentEpoch > 2029920906) {
    //    Serial.println("Inaccurate time: " + String(currentEpoch));
    return 0;
  }

  return currentEpoch;
}

void syncRtcTime() {
  Serial.print("Previous RTC time: ");
  Serial.println(rtc.now().unixtime());

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Tried syncRtcTime(): not connected to WiFi");
    return;
  }

  fetchText();

  bool updateSuccessful = false;
  const int maxRetries = sizeof(ntpServers) / sizeof(ntpServers[0]);

  for (int attempt = 0; attempt < maxRetries; attempt++) {
    changeNtpServer(attempt % maxRetries);
    Serial.println(currentNtpServer);

    if (timeClient.update()) {
      updateSuccessful = true;
      Serial.print("Successfully updated time from NTP server: ");
      Serial.println(currentNtpServer);
      break;
    }

    delay(200);  // Wait for 20 ms before retrying
    Serial.println("Retrying NTP update...");
  }

  if (updateSuccessful) {
    uint32_t ntpEpochTime = timeClient.getEpochTime();
    rtc.adjust(DateTime(ntpEpochTime));
    Serial.print("Adjusted RTC time: ");
    Serial.println(rtc.now().unixtime());
  } else {
    Serial.println("Failed to update time from servers.");
  }

  previousSynctimeMin = millis() / 60000;
}


void changeNtpServer(int serverIndex) {
  if (serverIndex < sizeof(ntpServers) / sizeof(ntpServers[0])) {
    timeClient.end();
    timeClient.setPoolServerName(ntpServers[serverIndex]);
    timeClient.begin();
    currentNtpServer = ntpServers[serverIndex]; // Update currentNtpServer with the new server address
  }
}

void formatTime(uint32_t epoch, char* buffer, size_t bufferSize) {
  // Convert epoch to time structure
  time_t rawtime = (time_t)epoch;
  rawtime += 5.5 * 3600; // Adjust for IST (UTC+5:30)
  struct tm* timeinfo = gmtime(&rawtime);

  // Format time into buffer with the desired format
  strftime(buffer, bufferSize, "%I:%M%p", timeinfo);  // Example: 08:59AM
}

void updateDisplayWithCurrentTime() {
  uint32_t currentEpoch = getCurrentEpoch();

  char timeString[8];  // Enough to hold "HH:MMAM"
  formatTime(currentEpoch, timeString, sizeof(timeString));
  sendText(timeAdd, timeString, 9);  // Use the fixed timeAdd address

//  Serial.print("Current Time: ");
//  Serial.println(timeString);  // Print the time string to the serial monitor
}
