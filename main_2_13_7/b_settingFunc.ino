void writeNewTempToEEPROM(float newTemp) {
  // Check if newTemp is within the valid range
  if (newTemp < 30.0 || newTemp > 50.0) {
    Serial.println("Error: New temperature value out of range (30.0 to 50.0)");
    return;
  }

  // Convert the temperature to the appropriate value for EEPROM storage
  uint8_t savedValue = static_cast<uint8_t>((newTemp - 30.0) * 10);

  // Write the value to EEPROM
  EEPROM.write(168, savedValue);
  EEPROM.commit();

  Serial.print("New temperature value ");
  Serial.print(newTemp, 1); // Print with one decimal place
  Serial.print(" written to EEPROM as ");
  Serial.println(savedValue);
  Serial.println();

  // Update other variables based on the new temperature
  setNewTempFromEEPROM();
}

void setNewTempFromEEPROM() {
  byte eepromValue = EEPROM.read(168);

  newTemp = 30.0 + (static_cast<float>(eepromValue) / 10.0); // Calculate newTemp using the formula

  // Update setTemp only if the EEPROM value is valid
  if (newTemp >= 30.0 && newTemp <= 50.0) {
    setTemp = newTemp; // Update setTemp
    MinTemp = setTemp - 5; // Update MinTemp
    MaxTemp = setTemp; // Update MaxTemp

    Serial.print("SetTemp updated from EEPROM: ");
    Serial.println(setTemp);

  } else {
    Serial.println("Invalid EEPROM value. Keeping the default temperature.");
  }

  // Convert setTemp to string
  char setTempStr[6]; // Assuming maximum 5 characters for temperature plus null terminator
  dtostrf(setTemp, 4, 1, setTempStr); // Convert float to string with one decimal place

  // Send setTemp to display
  sendText(setTempAdd, setTempStr, 4);
}



void setTemperature() {
  Serial.println("Enter the new temperature value in degrees Celsius (e.g., for 36.5°C, enter 36.5):");

  while (!Serial.available()) {
    delay(100); // Wait for input
  }

  // Read the input line from the Serial Monitor
  String input = Serial.readStringUntil('\n');

  // Convert the input string to a float
  float newTemp = input.toFloat();

  // Validate the input
  if (newTemp >= 30.0 && newTemp <= 50.0) { // Check if the input is within the range (30.0 to 50.0°C)
    writeNewTempToEEPROM(newTemp);
    Serial.println("Temperature value updated and saved to EEPROM.");
  } else {
    Serial.println("Invalid input. Temperature value must be between 30.0 and 50.0°C.");
  }
}

void settempDiff() {
  Serial.println("Enter the thermometer temperature value in degrees Celsius (e.g., for 36.5°C, enter 36.5):");

  while (!Serial.available()) {
    delay(100); // Wait for input
  }

  // Read the input line from the Serial Monitor
  String input = Serial.readStringUntil('\n');
//tempDiff = 0;
  // Convert the input string to a float
  float actualTemp = input.toFloat();

  // Validate the input
  if (actualTemp >= 25.0 && actualTemp <= 50.0) {  // Check if the input is within the range (30.0 to 50.0°C)
    Serial.println("old tempDiff = " + String(tempDiff));
    tempDiff = setTemp - tempDiff - actualTemp;
    Serial.println("new tempDiff = " + String(tempDiff));
    EEPROM.put(302, tempDiff);
    EEPROM.commit();
    Serial.println("Temperature difference updated and saved in memory.");
  } else {
    Serial.println("Invalid input. Actual temperature value must be between 25.0 and 50.0°C.");
  }

}

void writeRotationIntervalToEEPROM(int interval) {
  EEPROM.write(166, interval);
  EEPROM.commit();
}

void setRotationIntervalFromEEPROM() {
  int interval = EEPROM.read(166);

  char intervalStr[3]; // Assuming the integer value can be represented in up to 2 characters plus null terminator

  // Convert integer to string
  sprintf(intervalStr, "%d", interval);

  // Call sendText with the string
  sendText(rotationIntervalAdd, intervalStr, 2);

  if (interval) {
    rotationInterval = interval; // If interval is not equal to 0
    isRotationOn = true; // If interval is not 0
    Serial.print("Rotation interval updated from EEPROM: ");
    Serial.println(rotationInterval);
  }
  else
  {
    isRotationOn = false; // If interval is 0
    Serial.print("Rotation is off");
  }
}
