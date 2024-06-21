void sensorSetup() {
  pinMode(LIMIT_PIN, INPUT_PULLUP);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(MOT_DRIVER_PIN, OUTPUT);

  t0 = 298.15;  // Temperature t0 from datasheet, conversion from Celsius to Kelvin
  pinMode(HEATER_PIN, OUTPUT);

  // Rescale timer according to dt
  SetTime = SetTime * TicksPerMS;

  // Motor Driver Control
  digitalWrite(MOT_DRIVER_PIN, HIGH);

  // MUX Setup
  pinMode(muxS0, OUTPUT);
  pinMode(muxS1, OUTPUT);
  pinMode(muxS2, OUTPUT);
  pinMode(muxS3, OUTPUT);
  pinMode(COM1, OUTPUT);
  pinMode(COM2, OUTPUT);

  digitalWrite(muxS0, LOW);
  digitalWrite(muxS1, LOW);
  digitalWrite(muxS2, LOW);
  digitalWrite(muxS3, LOW);
  digitalWrite(COM1, LOW);
  digitalWrite(COM2, LOW);

  // PWM Setup
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(HEATER_PIN, pwmChannel);



  // Initialize each ADS1115 on the main I2C bus
  if (!adsTemp.begin(0x48, &I2Ctwo)) {
    Serial.println("Failed to initialize adsTemp.");
    sendText(logBoxAdd, "Failed to initialize adsTemp.", 100);
    while (1);
  }

  if (!ads1.begin(0x49, &I2Cone)) {
    Serial.println("Failed to initialize ads1.");
    sendText(logBoxAdd, "Failed to initialize ads1.", 100);
    while (1);
  }
  if (!ads2.begin(0x4B, &I2Cone)) {
    Serial.println("Failed to initialize ads2.");
    sendText(logBoxAdd, "Failed to initialize ads2.", 100);
    while (1);
  }
  if (!ads3.begin(0x48, &I2Cone)) {
    Serial.println("Failed to initialize ads3.");
    sendText(logBoxAdd, "Failed to initialize ads3.", 100);
    while (1);
  }
  if (!ads4.begin(0x4A, &I2Cone)) {
    Serial.println("Failed to initialize ads4.");
    sendText(logBoxAdd, "Failed to initialize ads4.", 100);
    while (1);
  }

  // Set gain and data rate for all ADS1115 devices
  ads1.setGain(GAIN_ONE);
  ads2.setGain(GAIN_ONE);
  ads3.setGain(GAIN_ONE);
  ads4.setGain(GAIN_ONE);
  adsTemp.setGain(GAIN_ONE);

  ads1.setDataRate(RATE_ADS1115_8SPS);
  ads2.setDataRate(RATE_ADS1115_8SPS);
  ads3.setDataRate(RATE_ADS1115_8SPS);
  ads4.setDataRate(RATE_ADS1115_8SPS);
  adsTemp.setDataRate(RATE_ADS1115_8SPS);

  readBestGainFromEEPROM();

  controlTemp();

  sendText(temperatureAdd, String(temperature, 1).c_str(), 4); // Send temperature

  readFromEEPROM(userEmail, 169, 50);
  Serial.print("User Email: ");
  Serial.println(userEmail);

  readFromEEPROM(userPassword, 219, 50);
  Serial.print("User Password: ");
  Serial.println(userPassword);

  printwhite(); // Print white values on serial
  setRotationIntervalFromEEPROM();
  setNewTempFromEEPROM();
  EEPROM.get(301, rotationAngle);
  //  EEPROM.get(302, tempDiff);

  previousRotationEpoch = getCurrentEpoch();
}

void controlTemp() {
  temperature = adsTemp.readADC_SingleEnded(0); //adc value
  temperature = static_cast<float>(Thermistor(static_cast<double>(temperature)));//adc to Celcius
  temperature = temperature; // - tempDiff; // if actual temp = 35C but displayed temp is 37C, then tempDiff = 2C = Displayed - Actual
  Control_PID(temperature);
  fetchText();
}


void rotate_()
{
  Serial.println("motor rotate");
  if (digitalRead(LIMIT_PIN) == HIGH) motorRotate(0, 16 * 15); // If limitswitch is not pressed then go towards the limitswitch, else don't
  motorRotate(1, 16 * 53); //away from limitswitch
  motorRotate(0, 16 * (53 + 3)); //towards the limitswitch
  digitalWrite(MOT_DRIVER_PIN, HIGH); //motor deactivated
  previousRotationEpoch = getCurrentEpoch();
  savePreviousRotationEpoch();
}


void motorRotate(int dir, int stepcount)
{
  digitalWrite(DIR_PIN, dir);
  digitalWrite(MOT_DRIVER_PIN, LOW); //motor activated
  // Makes 200 pulses for making one full cycle rotations
  for (int x = 0; x < stepcount; x++) {

    if (digitalRead(LIMIT_PIN) == LOW && dir == 0) //LOW when pressed
    {
      digitalWrite(MOT_DRIVER_PIN, HIGH);//motor deactivated
      return; //false when unpressed i.e. keep continuing
    }

    digitalWrite(STEP_PIN, HIGH);
    delay(3);    // more delay, slower speed and higher torque
    digitalWrite(STEP_PIN, LOW);
    delay(3);
  }
  delay(1000); // One second delay
}

void ledPinSelect(int channel, bool state) {

  int controlPin[] = {muxS0, muxS1, muxS2, muxS3};
  int comPin;

  if (channel < numChannels) comPin = COM1;
  else comPin = COM2;

  digitalWrite(comPin, 0); //first turn off the light

  channel = channel % numChannels;

  int muxChannel[16][4] = {
    {1, 0, 0, 0}, // channel 0 R1
    {0, 0, 0, 0}, // channel 1 IR1
    {1, 1, 0, 0}, // channel 2 R2
    {0, 1, 0, 0}, // channel 3 IR2

    {1, 0, 1, 0}, // channel 4 R3
    {0, 0, 1, 0}, // channel 5 IR3
    {1, 1, 1, 0}, // channel 6 R4
    {0, 1, 1, 0}, // channel 7 IR4

    {1, 0, 0, 1}, // channel 8 R5
    {0, 0, 0, 1}, // channel 9 IR5
    {1, 1, 0, 1}, // channel 10 R6
    {0, 1, 0, 1}, // channel 11 IR6

    {1, 0, 1, 1}, // channel 12 R7
    {0, 0, 1, 1}, // channel 13 IR7
    {1, 1, 1, 1}, // channel 14 R8
    {0, 1, 1, 1}  // channel 15 IR8
  };


  //loop through the 4 sig
  for (int i = 0; i < 4; i++) {
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  delay(60); //important delay

  if (!testingPhotodiode) digitalWrite(comPin, state);
  else digitalWrite(comPin, 0);
  delay(1);
}

void rawdata()
{
  Serial.print(",");

  for (int i = 0; i < numChannels * 2 ; i++)
  {
    Serial.print(sensorRead(i));
    Serial.println(",");
  }

  Serial.println();
}

void takeWhite(int chID)
{
  Serial.print("Whiting channel " + String(chID + 1) );
  for (int i = 0; i < 2 ; i++)
  {
    uint16_t intensity = sensorRead(chID * 2 + i);

    if (intensity < 5000) {
      Serial.println(":  White <5000, not saving it.");  //if white intensity is not valid, don't save values
      sendText(0x1980, ("Error: Channel " + String(chID + 1)).c_str(), 30);
      return;
    }

    else if (intensity > 32500) {
      Serial.println(":  White saturated, not saving it.");  //if white intensity is not valid, don't save values
      sendText(0x1980, ("Saturated: " + String(chID + 1)).c_str(), 30);
      return;
    }

    whiteIntensity[chID * 2 + i] = intensity;
    //    Serial.println(String(whiteIntensity[chID * 2 + i]) + ",");

    byte quotient = whiteIntensity[chID * 2 + i] / 255;  // Calculate quotient (0-254)
    byte remainder = whiteIntensity[chID * 2 + i] % 255; // Calculate remainder (0-254)
    EEPROM.write(chID * 4 + i * 2, quotient); // Store quotient
    EEPROM.write(chID * 4 + i * 2 + 1, remainder); // Store remainder
  }
  EEPROM.commit();

  Serial.println("  White updated succesfully for channel");
  sendText(0x1980, ("White " + String(chID + 1) + " calibration complete!").c_str(), 30);

  return;
}

//Thermistor code
double Thermistor(double VRT) {
  double Temp;
  VRT   = (VRT / ADCmax) * VCC;      //Conversion to voltage 26255
  VR = VCC - VRT;
  RT = VRT / (VR / Ra);               //Resistance of RT
  ln = log(RT / Rt0);
  Temp = (1 / ((ln / Ba) + (1 / t0))); //Temperature from thermistor
  Temp = Temp - 273.15;

  return Temp;
}

void Control_PID(float iTemp) {

  //Overheat protection
  if (iTemp > MaxTemp) {
    ledcWrite(pwmChannel, 0);
    //Serial.println("Error:overheat. Heater turned off");
    return;
  }

  //In range? If in range, maybe turn on LED?
  if ((iTemp) >= setTemp) {

    if (bInRange == 0) {
      //digitalWrite(ledPin1, HIGH);

      bInRange = 1;
    }
  } else {
    if (bInRange == 1) {

      //digitalWrite(ledPin1, LOW);
      bInRange = 0;
    }

  }


  //PID subroutine
  float err = setTemp - iTemp;
  //Serial.println(err);
  s_integral += err * dt;
  //Serial.println(s_integral);
  float s_derivative = (err - previous_error) / dt;
  //Serial.println(s_derivative);
  int U_in_ctrl = (K_P_ctrl * err + K_I_ctrl * s_integral + K_D_ctrl * s_derivative) / (MaxTemp - MinTemp) * 1023;
  //if (U_in_ctrl > 700) U_in_ctrl = 700;

  previous_error = err;


  // put voltage to output and write value to serial monitor
  //  Serial.print("Output PWM frequency: ");

  if (U_in_ctrl <= 1023) {
    if (U_in_ctrl > 0) {
      ledcWrite(pwmChannel, U_in_ctrl);
      //     Serial.println(U_in_ctrl);

    }
    else
    {
      ledcWrite(pwmChannel, 1);
      //    Serial.println("1 / 0 V");
    }
  }
  else {
    ledcWrite(pwmChannel, 1023); //ledcWrite(pwmChannel,700);

    //   Serial.println("1023 / 3.3 V");
  }

}


void printwhite() {
  Serial.print("White values: ");
  for (int i = 0; i < numChannels * 2; i++)
  {
    byte quotient = EEPROM.read(2 * i);          // Read quotient
    byte remainder = EEPROM.read(2 * i + 1);     // Read remainder

    whiteIntensity[i] = quotient * 255 + remainder;   // Combine quotient and remainder
    Serial.print(whiteIntensity[i]); Serial.print(" ,");
  }

  Serial.println();
}

void setRotationInterval() {
  Serial.println("Enter the new rotation interval in minutes:");

  while (!Serial.available()) {
    delay(100); // Wait for input
  }

  // Read the input line from the Serial Monitor
  String input = Serial.readStringUntil('\n');

  // Convert the input string to an integer
  int newInterval = input.toInt();

  // Validate the input
  if (newInterval != 0 || input.startsWith("0")) {
    // Input is valid, proceed to save to EEPROM
    if (newInterval >= minRotationInterval && newInterval <= maxRotationInterval) {
      writeRotationIntervalToEEPROM(newInterval);
      rotationInterval = newInterval; // Update the current rotation interval
      Serial.println("Rotation interval updated and saved to EEPROM."); Serial.println(rotationInterval);
    } else {
      Serial.println("Invalid input. Rotation interval must be between 15 and 300 minutes.");
    }
  } else {
    // Input is not valid
    Serial.println("Invalid input format. Please enter a valid rotation interval.");
  }
}

void setSerial() {
  Serial.println("Enter the new serial value:");

  while (!Serial.available()) {
    delay(100); // Wait for input
  }

  // Read the input value from the Serial Monitor
  serialNum = Serial.parseInt();

  // Print the entered serial value for debugging
  Serial.print("Entered serial value: ");
  Serial.println(serialNum);

  // Extract quotient and remainder
  byte quotient = serialNum / 255;
  byte remainder = serialNum % 255;

  // Save quotient and remainder to EEPROM
  EEPROM.write(64, quotient);
  EEPROM.write(65, remainder);
  EEPROM.commit();

  Serial.println("Serial value updated and saved to EEPROM.");

  deviceID = "MBS" + String(serialNum + 100000).substring(1);
  Serial.println(deviceID);
}

void setSSID() {
  Serial.println("Enter new SSID:");
  while (!Serial.available()) {
    delay(100);
  }

  // Read SSID from Serial input
  int i = 0;
  while (Serial.available() && i < sizeof(ssid) - 1) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      break;
    }
    ssid[i++] = c;
  }
  ssid[i] = '\0'; // Null-terminate the SSID string

  Serial.print("New SSID: ");
  Serial.println(ssid);

  // Write SSID to EEPROM
  saveToEEPROM(ssid, 66, 50);

  Serial.println("SSID updated and saved to EEPROM.");
  checkWiFiConnection();
}

void setPassword() {
  Serial.println("Enter new Password:");
  while (!Serial.available()) {
    delay(100);
  }

  // Read password from Serial input
  int i = 0;
  while (Serial.available() && i < sizeof(password) - 1) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      break;
    }
    password[i++] = c;
  }
  password[i] = '\0'; // Null-terminate the password string

  Serial.print("New Password: ");
  Serial.println(password);

  // Write password to EEPROM
  saveToEEPROM(password, 116, 50);

  Serial.println("Password updated and saved to EEPROM.");
}


void setUser() {
  Serial.println("Enter new User ID:");
  while (!Serial.available()) {
    delay(100);
  }
  String newUser = Serial.readStringUntil('\n');
  Serial.println("New User ID: " + newUser);

  // Write User to EEPROM
  saveToEEPROM(newUser.c_str(), 169, 50);

  Serial.println("User ID updated and saved to EEPROM.");
}

void setUserPass() {
  Serial.println("Enter new User Password:");
  while (!Serial.available()) {
    delay(100);
  }
  String newPass = Serial.readStringUntil('\n');
  Serial.println("New User Password:: " + newPass);

  // Write SSID to EEPROM
  saveToEEPROM(newPass.c_str(), 219, 50);

  Serial.println("User Password updated and saved to EEPROM.");
}

int extractChannelID(String command) {
  // Extract the channel ID from the "start#X", "stop#X", or "white#X" command
  if (command.startsWith("start#") || command.startsWith("stop#") || command.startsWith("white#")) {
    return command.substring(6).toInt();
  }
  return -1; // Return -1 if the command doesn't contain a valid channel ID
}



void readChannels() {
  uint32_t currentEpoch = getCurrentEpoch();

  if (currentEpoch - lastDisplayTime >= datainterval * 60 && currentEpoch - previousRotationEpoch > 1 * 60 ) {
    //    Serial.println(String(currentEpoch) + ", " + String(lastDisplayTime) + ", " + String(currentEpoch - lastDisplayTime));
    lastDisplayTime = currentEpoch;

    //    Serial.print(",");

    for (int i = 0; i < numChannels; i++) {
      if (isRunning[i]) {

        latestIntensity[2 * i] = sensorRead(2 * i);
        latestIntensity[2 * i  + 1] = sensorRead(2 * i + 1);

        fetchText();

        //------------- calculating progressValues below ---------------

        if (latestIntensity[2 * i] < 3000 || latestIntensity[2 * i  + 1] < 3000) {
          progressValues[i] = 0;
        }

        else {

          float abs1 = log10 ( (float) whiteIntensity[2 * i] / latestIntensity[2 * i] );
          float abs2 = log10 ( (float) whiteIntensity[2 * i + 1] / latestIntensity[2 * i + 1]  );
          long absVal = round(100000 * ( abs1 - abs2 ));

          progressValues[i] = round(1000 * ( (float)(firstReading[i] - absVal) / firstReading[i] ) );

          if (progressValues[i] < -69) {
            firstReading[i] = absVal;
            modifySavedChannel(i + 1, latestIntensity[2 * i], latestIntensity[2 * i + 1], firstReading[i]);
          }
        }

        //        Serial.print(", progress: ");
        if (logflag) {
          Serial.print(progressValues[i]);
          Serial.print(",");
        }


        if (progressValues[i] > progressThreshold * 10) {

          progressAvg[i] = round((progressValues[i] + progressHistory[i][0] + progressHistory[i][1] + progressHistory[i][2]) / 4);

          // Shift progress history
          progressHistory[i][2] = progressHistory[i][1];
          progressHistory[i][1] = progressHistory[i][0];
          progressHistory[i][0] = progressValues[i];

          if (progressAvg[i] <= lastProgressAvg[i]) {
            progressValues[i] =  round(progressValues[i] / 10);
            completedChannel(false, i + 1, 0); //0 is ignored
            continue;
          }

          lastProgressAvg[i] = progressAvg[i];
        }

        progressValues[i] =  round(progressValues[i] / 10);


        //      Manipulations For Display
        if (progressValues[i] > 99) progressValues[i] = 99;
        if (progressValues[i] < 0) progressValues[i] = 0;

        updateProgress(i + 1);
        updateElapsedTime(i + 1 , 0);  //ignore 5, its not being used. **Dont Call it after completedChannel().**

        //---------------------------------

      }

      else {
        if (logflag) {
          Serial.print(0);
          Serial.print(",");
        }
      }
    }
    Serial.println();
  }
  fetchText();
}

long takeFirstReading(int chID) {
  fetchText();

  resetProgressHistory(chID);

  int i = chID * 2;

  first1 = sensorRead(i);
  first2 = sensorRead(i + 1);

  if ( 100 * ((float)abs(whiteIntensity[i + 1] - first2) / min (whiteIntensity[i + 1], first2)) > 10 ) {
    Serial.println("First2 deviated by > 10%. Restart/Re-white " + String(first2) + ", " + String(whiteIntensity[i + 1]));
    sendText(nameArray[chID], "Restart/Re-white", 16);
    beep(500);
    return -2147483648;
  }

  float abs1 = log10 ( (float) whiteIntensity[i] / first1 );
  float abs2 = log10 ( (float) whiteIntensity[i + 1] / first2 );

  //  firstReading[chID] = round( 100000 * ( log10((float)whiteIntensity[i] / first1 ) - log10((float)whiteIntensity[i + 1] / first2) ) );
  firstReading[chID] = round( 100000 * ( abs1 - abs2 ) );

  Serial.print(" First Reading = "); Serial.println(firstReading[chID]);
  progressValues[chID] = 0;

  return firstReading[chID];
}

void resetProgressHistory(int i) {
  for (int j = 0; j < 3; j++) {
    progressHistory[i][j] = 0;
  }
  lastProgressAvg[i] = 0;
}


void setGainChannel(int i) {
  if (i >= 1 && i <= 16) {
    determineBestGainForLED(2 * (i - 1));
    determineBestGainForLED(2 * (i - 1) + 1);
    takeWhite(i - 1);
  }
}


void setGainAll() {
  for (int i = 1; i <= 16; i++)
  {
    determineBestGainForLED(2 * (i - 1)); // Set gain for the first LED in the pair
    determineBestGainForLED(2 * (i - 1) + 1); // Set gain for the second LED in the pair
    takeWhite(i - 1);
  }
}


void blue(int channelID) {

  uint16_t blue1 = sensorRead(2 * (channelID - 1));
  uint16_t blue2 = sensorRead(2 * (channelID - 1) + 1);

  //  Serial.println("blue1 = " + String(blue1));
  //  Serial.println("blue2 = " + String(blue2));

  float abs1 = log10((float)whiteIntensity[2 * (channelID - 1)] / blue1);
  float abs2 = log10((float)whiteIntensity[2 * (channelID - 1) + 1] / blue2);
  int contrast = round(10000 * (abs1 - abs2));

  if (contrast < minContrast) {
    Serial.println("Channel: " + String(channelID) + " Low contrast" + ", Blue1: " + String(blue1) + ", White1: " + String(whiteIntensity[2 * (channelID - 1)]) + ", Contrast: " + String(contrast));
    String errorMessage = ", Low contrast: " + String(channelID);
    Serial.println(errorMessage);

    sendText(diagnosticBox, errorMessage.c_str(), 100);
    beep(500);
    if (channelID == 16) delay(100);
  }
}

void blueAll() {
  for (int i = 1; i <= 16; i++) {
    blue(i);
  }
  String errorMessage = "Blue Check Complete!";
  Serial.println(errorMessage);
  sendText(diagnosticBox, errorMessage.c_str(), 100);
  beep(500);
}
