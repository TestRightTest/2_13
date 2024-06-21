void spiffSetup() {
  if (!SPIFFS.begin()) {
    SPIFFS.format();
    if (!SPIFFS.begin()) {
      Serial.println("Failed to mount SPIFFS");
      sendText(logBoxAdd, "Failed to mount SPIFFS", 100);
      delay(1000);
      return;
    }
  }

  loadPreviousRotationEpoch();
  checkChannelsToResume();
  checkWiFiConnection();
}

void saveStartChannel(uint8_t channelID_, const char* sampleName_, uint32_t startTimeEpoch_, long firstReading_, uint16_t white1_, uint16_t white2_, uint16_t first1_, uint16_t first2_) {
  // Construct the filename based on the provided testCount and channelID
  String fileName = "/" + String(testCount) + "_" + String(channelID_) + ".bin";

  // Check if the file already exists
  if (SPIFFS.exists(fileName)) {
    Serial.println("File already exists.");
    return;
  }

  // Open the file in write mode (create new file)
  File file = SPIFFS.open(fileName, "w");
  if (!file) {
    Serial.print("Failed to create file for writing. Filename: ");
    Serial.println(fileName);
    return;
  }

  // Find and delete the oldest file if the maximum number of files is reached
  if (testCount > MAX_TEST_COUNT) {
    deleteOldestFile();
  }

  // Create and fill the new data struct
  channelData data;
  data.channelID = channelID_;
  strncpy(data.sampleName, sampleName_, sizeof(data.sampleName) - 1); // Copy sampleName, ensuring null termination
  data.startTimeEpoch = startTimeEpoch_;
  data.firstReading = firstReading_;

  data.white1 = white1_;
  data.white2 = white2_;
  data.first1 = first1_;
  data.first2 = first2_;

  // Initialize endTimeEpoch and progressValue to 0
  data.endTimeEpoch = 0;
  data.progressValue = 0;
  data.last1 = 0;
  data.last2 = 0;
  data.isCompleted = false;
  sendStartChannelDataToServer(data);//send start data to server


  String StartData = "Resuming Channel: " + String(data.channelID) + ", Sample Name: " + data.sampleName + ", First Reading: " + String(data.firstReading) + ", Start Time Epoch: " + String(data.startTimeEpoch);
  sendDataToDesktop(StartData.c_str()); //start channel data to wap desktop



  // Write the new data to the file
  if (file.write((uint8_t*)&data, sizeof(channelData)) != sizeof(channelData)) {
    Serial.println("Failed to write data to file");
  } else {
    Serial.println("Channel data saved in memory.");
  }

  // Close the file
  file.close();
}


void saveStopChannel(uint8_t channelID, uint32_t endTimeEpoch, uint8_t progressValue, uint16_t last1_, uint16_t last2_, bool isCompleted_) {
  // Construct the filename based on the channelID
  String fileName = "/" + String(latestTestCounts[channelID - 1]) + "_" + String(channelID) + ".bin";

  // Open the file in write mode
  File file = SPIFFS.open(fileName, "r+");

  if (!file) {
    Serial.println("Failed to open file for reading and writing");
    return;
  }

  // Find the start position of the struct data for the given channelID
  size_t startPos = 0;
  channelData data;
  while (file.available()) {
    size_t bytesRead = file.read((uint8_t*)&data, sizeof(channelData));
    if (bytesRead == sizeof(channelData) && data.channelID == channelID) {
      break;
    }
    startPos += bytesRead;
  }

  // Move the file pointer to the start position of the struct data
  file.seek(startPos);


  if (isRunning[channelID - 1] == true) {
    // Update the endTimeEpoch and progressValue fields of the struct data
    data.endTimeEpoch = endTimeEpoch;
    data.progressValue = progressValue;
    data.last1 = last1_;
    data.last2 = last2_;
    data.isCompleted = isCompleted_;
    sendEndChannelDataToServer(data);

    String jsonData;
    jsonData += "{";
    jsonData += "\"channelID\": " + String(data.channelID) + ",";
    jsonData += "\"endTimeEpoch\": " + String(data.endTimeEpoch) + ",";
    jsonData += "\"progressValue\": \"" + String(data.progressValue) + "\",";
    jsonData += "}";

    // Send the JSON data to the desktop
    sendDataToDesktop(jsonData.c_str());


  }

  else if (isRunning[channelID - 1] == false && isCompleted[channelID - 1] == false) {
    data.isCompleted = isCompleted_;
  }



  // Write the modified data back to the file
  if (file.write((uint8_t*)&data, sizeof(channelData)) != sizeof(channelData)) {
    Serial.println("Failed to write data to file");
  } else {
    //    Serial.println("Channel data updated in memory.");
  }
  file.close();
}

void modifySavedChannel(uint8_t channelID, uint16_t newFirst1, uint16_t newFirst2, long firstReading) {
  // Construct the filename based on the channelID
  String fileName = "/" + String(latestTestCounts[channelID - 1]) + "_" + String(channelID) + ".bin";

  // Open the file in read-write mode
  File file = SPIFFS.open(fileName, "r+");

  if (!file) {
    Serial.println("Failed to open file for reading and writing");
    return;
  }

  // Find the start position of the struct data for the given channelID
  size_t startPos = 0;
  channelData data;
  while (file.available()) {
    size_t bytesRead = file.read((uint8_t*)&data, sizeof(channelData));
    if (bytesRead == sizeof(channelData) && data.channelID == channelID) {
      break;
    }
    startPos += bytesRead;
  }

  // Move the file pointer to the start position of the struct data
  file.seek(startPos);

  // Update the first1 and first2 fields of the struct data
  data.first1 = newFirst1;
  data.first2 = newFirst2;
  data.firstReading = firstReading;

  // Write the modified data back to the file
  file.seek(startPos);
  if (file.write((uint8_t*)&data, sizeof(channelData)) != sizeof(channelData)) {
    Serial.println("Failed to write data to file");
  } else {
    Serial.print("Channel " + String(channelID) + " data modified in memory. ");
  }
  file.close();
}


void checkChannelsToResume() {

  updateTestCount();

  for (int channelID = 1; channelID <= 16; channelID++) {

    if (latestTestCounts[channelID - 1]) {

      String fileName = "/" + String(latestTestCounts[channelID - 1]) + "_" + String(channelID) + ".bin";

      // Check if the file exists before attempting to open it
      if (SPIFFS.exists(fileName)) {
        File channelFile = SPIFFS.open(fileName, FILE_READ);

        if (channelFile) {
          // Read data from the file
          channelData data;
          size_t bytesRead = channelFile.read((uint8_t*)&data, sizeof(channelData));


          if (bytesRead == sizeof(channelData)) {

            if (data.endTimeEpoch == 0 && data.channelID == channelID) {

              String sampleName = String(data.sampleName);
              long firstReading = data.firstReading;
              uint32_t startTimeEpoch = data.startTimeEpoch;
              uint32_t endTimeEpoch = data.endTimeEpoch;
              uint8_t progressValue = data.progressValue;
              latestIntensity[2 * (channelID - 1) + 1] = data.first2;

              isRunning[channelID - 1] = true;
              startResumeChannel(true, channelID, sampleName.c_str(), firstReading, startTimeEpoch);

              // Send resume data to the desktop
              String resumeData = "Resuming Channel: " + String(channelID) + ", Sample Name: " + sampleName + ", First Reading: " + String(firstReading) + ", Start Time Epoch: " + String(startTimeEpoch);
              sendDataToDesktop(resumeData.c_str());
            }

            else if (data.isCompleted == true && data.channelID == channelID) {

              String sampleName = String(data.sampleName);
              uint16_t decolorTimeMin = (data.endTimeEpoch - data.startTimeEpoch) / 60;

              sendText(nameArray[channelID - 1], sampleName.c_str(), 16);

              completedChannel(true, channelID, decolorTimeMin);
            }

          } else {
            Serial.println("Error reading data from file: " + fileName);
          }

          channelFile.close();
        }
      } else {
        Serial.println("No file found with name: " + fileName);
      }
    }
  }
}

void updateTestCount() {
  // Initialize oldestTestCount to the maximum possible value
  oldestTestCount = UINT32_MAX;

  // Open SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    sendText(logBoxAdd, "Failed to mount SPIFFS", 100);
    return; // Return 0 if SPIFFS failed to mount
  }

  // Open directory
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  // Iterate over files in SPIFFS
  while (file) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      // Check if the filename matches the pattern
      if (fileName.endsWith(".bin")) {
        // Extract the numerical parts
        int dotIndex = fileName.indexOf('.');
        int underscoreIndex = fileName.indexOf('_');
        String countStr = fileName.substring(0, underscoreIndex);
        String channelIDStr = fileName.substring(underscoreIndex + 1, dotIndex);
        uint32_t count = countStr.toInt();
        uint8_t channelID = channelIDStr.toInt();
        // Update the latest testCount if necessary
        if (count > testCount) {
          testCount = count;
        }
        // Update oldestTestCount
        if (count < oldestTestCount) {
          oldestTestCount = count;
          oldestChannelID = channelID;
        }
        // Update latestTestCounts
        if (count > latestTestCounts[channelID - 1]) {
          latestTestCounts[channelID - 1] = count;
        }
      }
    }
    file = root.openNextFile();
  }

  // Close directory
  root.close();

  Serial.print("testCount = "); Serial.println(testCount);
  Serial.print("oldestTestCount = "); Serial.println(oldestTestCount);

  Serial.println("Latest Test Counts:");
  for (int i = 0; i < 16; i++) {
    Serial.print("Channel ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(latestTestCounts[i]);
  }
}

void clearSpiffData() {
  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully. All data cleared.");
  } else {
    Serial.println("Failed to format SPIFFS.");
  }
}

size_t getTotalSpiffsBytes() {
  // Open SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return 0;
  }

  // Get total bytes
  size_t totalBytes = SPIFFS.totalBytes();

  // Close SPIFFS
  SPIFFS.end();

  return totalBytes;
}

size_t getUsedSpiffsBytes() {
  // Open SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return 0;
  }

  // Get used bytes
  size_t usedBytes = SPIFFS.usedBytes();

  // Close SPIFFS
  SPIFFS.end();

  return usedBytes;
}

size_t getRemainingSpiffsBytes() {
  // Calculate remaining bytes
  size_t totalBytes = getTotalSpiffsBytes();
  size_t usedBytes = getUsedSpiffsBytes();
  return totalBytes - usedBytes;
}

void displaySpiffsUsage() {
  // Display total, used, and remaining bytes
  Serial.print("Total SPIFFS memory: ");
  Serial.print(getTotalSpiffsBytes());
  Serial.println(" bytes");
  Serial.print("Used SPIFFS memory: ");
  Serial.print(getUsedSpiffsBytes());
  Serial.println(" bytes");
  Serial.print("Remaining SPIFFS memory: ");
  Serial.print(getRemainingSpiffsBytes());
  Serial.println(" bytes");
}

void deleteOldestFile() {
  // Open SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Attempt to delete files until the oldest file is found
  while (true) {
    // Construct the filename based on the oldest test count
    String fileName = "/" + String(oldestTestCount) + "_" + String(oldestChannelID) + ".bin";

    // Check if the file exists
    if (SPIFFS.exists(fileName)) {
      // Delete the oldest file
      if (SPIFFS.remove(fileName)) {
        Serial.println("Oldest file deleted successfully");
        updateTestCount();
        return;
      } else {
        Serial.println("Failed to delete oldest file");
        return;
      }
    } else {
      updateTestCount();
      Serial.println("Oldest file not found, updatingTestcount().");
    }
  }
}


void readSpiffData() {
  Serial.println("Reading data from SPIFFS:");

  // Open SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Open directory
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  // Iterate over files in SPIFFS
  while (file) {
    if (!file.isDirectory()) {
      // Read data from the file
      Serial.print("File: ");
      Serial.println(file.name());
      channelData data;
      size_t bytesRead = file.read((uint8_t*)&data, sizeof(channelData));
      if (bytesRead == sizeof(channelData)) {
        // Print the data
        Serial.print("Channel ID: ");
        Serial.println(data.channelID);
        Serial.print("Sample Name: ");
        Serial.println(data.sampleName);
        Serial.print("Start Time Epoch: ");
        Serial.println(data.startTimeEpoch);
        Serial.print("End Time Epoch: ");
        Serial.println(data.endTimeEpoch);
        if (data.endTimeEpoch) {
          Serial.print("Decolor Time Min: ");
          Serial.println((data.endTimeEpoch - data.startTimeEpoch) / 60);
        }
        Serial.print("First Reading: ");
        Serial.println(data.firstReading);
        Serial.print("Progress Value: ");
        Serial.println(data.progressValue);
        Serial.print("White 1: ");
        Serial.println(data.white1);
        Serial.print("White 2: ");
        Serial.println(data.white2);
        Serial.print("First 1: ");
        Serial.println(data.first1);
        Serial.print("First 2: ");
        Serial.println(data.first2);
        Serial.print("Last 1: ");
        Serial.println(data.last1);
        Serial.print("Last 2: ");
        Serial.println(data.last2);
        Serial.print("Is completed: ");
        Serial.println(data.isCompleted);
        Serial.println();
      } else {
        Serial.println("Failed to read data from file");
      }
    }
    // Close the file
    file.close();
    // Open the next file
    file = root.openNextFile();
  }

  // Close directory
  root.close();
}



void savePreviousRotationEpoch() {
  File file = SPIFFS.open("/previousRotationEpoch.txt", "w");
  if (!file) {
    Serial.println("Failed to create previousRotationEpoch.txt file for writing");
    return;
  }
  file.println(previousRotationEpoch);
  file.close();
  Serial.print("PreviousRotationEpoch saved in spiff: "); Serial.println(previousRotationEpoch);

}

void loadPreviousRotationEpoch() {
  File file = SPIFFS.open("/previousRotationEpoch.txt", "r");
  if (!file) {
    Serial.println("Failed to open previousRotationEpoch.txt file for reading");
    return;
  }
  previousRotationEpoch = file.readStringUntil('\n').toInt();
  file.close();
  Serial.print("PreviousRotationEpoch loaded from spiff: "); Serial.println(previousRotationEpoch);
}
