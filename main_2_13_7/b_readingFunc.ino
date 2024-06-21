//void fillSpiff() {
//
//  for (int i = 0; i < 2650; i++)
//  {
//    testCount++;
//    startResumeChannel(false, (i % 16) + 1, "Test", 10000, 1710090806);
//    // saveStopChannel( (i%16)+1, 1710090906, 97);
//  }
//}
//


int sensorRead(int ledID) {

  ledID = channelarrangement[ledID]; //remapping the Led

  int channel = ledID / 2;
  int adsID = channel / 4 + 1;
  int adsChannel = channel % 4;

  adsGain_t LEDgain = gainMapping[bestGain[ledID]];

  switch (adsID) {
    case 1: ads1.setGain(LEDgain); break;
    case 2: ads2.setGain(LEDgain); break;
    case 3: ads3.setGain(LEDgain); break;
    case 4: ads4.setGain(LEDgain); break;
    default: break;
  }

  int dark = 0;
  if (!testSensorFlag) {
    switch (adsID) {
      case 1: dark = ads1.readADC_SingleEnded(adsChannel); break;
      case 2: dark = ads2.readADC_SingleEnded(adsChannel); break;
      case 3: dark = ads3.readADC_SingleEnded(adsChannel); break;
      case 4: dark = ads4.readADC_SingleEnded(adsChannel); break;
      default: break;
    }
  }

  fetchText();

  int sensorValue = 0;
  if (!testSensorFlag) ledPinSelect(ledID, HIGH);

  switch (adsID) {
    case 1: sensorValue = ads1.readADC_SingleEnded(adsChannel); break;
    case 2: sensorValue = ads2.readADC_SingleEnded(adsChannel); break;
    case 3: sensorValue = ads3.readADC_SingleEnded(adsChannel); break;
    case 4: sensorValue = ads4.readADC_SingleEnded(adsChannel); break;
    default: break;
  }

  ledPinSelect(ledID, LOW);

  if (testSensorFlag) Serial.print(String(sensorValue - dark) + ",");

  //  Serial.print(String(sensorValue) + " - " + String(dark) + " = " + String(sensorValue - dark) + ",");
  return sensorValue - dark;
}

void determineBestGainForLED(int ledID) {

  int actualChannel = ledID / 2 + 1;

  ledID = channelarrangement[ledID]; // Remapping the LED

  int channel = ledID / 2;
  int adsID = channel / 4 + 1;
  int adsChannel = channel % 4;
  int maxSensorValue = 0;
  adsGain_t currentBestGain = GAIN_ONE; // Default to the lowest gain initially
  int sensorValue = 0;
  int darkValue = 0;

  for (adsGain_t gain : gainMapping) {

    switch (adsID) {
      case 1:
        ads1.setGain(gain);
        darkValue = ads1.readADC_SingleEnded(adsChannel);
        break;
      case 2:
        ads2.setGain(gain);
        darkValue = ads2.readADC_SingleEnded(adsChannel);
        break;
      case 3:
        ads3.setGain(gain);
        darkValue = ads3.readADC_SingleEnded(adsChannel);
        break;
      case 4:
        ads4.setGain(gain);
        darkValue = ads4.readADC_SingleEnded(adsChannel);
        break;
      default:
        break; // Handle unexpected adsID
    }

    if (gain == GAIN_ONE && darkValue > 3000) {
      String errorMessage = String(actualChannel) + ": 100k Resistor Error";
      Serial.println(errorMessage);
      sendText(diagnosticBox, errorMessage.c_str(), 100);
      beep(500);
      //delay(100);
      return; // Exit loop if sensor value too high
    }

    ledPinSelect(ledID, HIGH);

    switch (adsID) {
      case 1:
        ads1.setGain(gain);
        sensorValue = ads1.readADC_SingleEnded(adsChannel);
        break;
      case 2:
        ads2.setGain(gain);
        sensorValue = ads2.readADC_SingleEnded(adsChannel);
        break;
      case 3:
        ads3.setGain(gain);
        sensorValue = ads3.readADC_SingleEnded(adsChannel);
        break;
      case 4:
        ads4.setGain(gain);
        sensorValue = ads4.readADC_SingleEnded(adsChannel);
        break;
      default:
        break; // Handle unexpected adsID
    }

    ledPinSelect(ledID, LOW);

    sensorValue = sensorValue - darkValue;

    if (gain == GAIN_ONE && sensorValue >= 27000) {

      if (sensorValue > 28000) {
        String errorMessage = String(actualChannel) + ": sensorValue too high at gain one!";
        Serial.println(errorMessage);
        sendText(diagnosticBox, errorMessage.c_str(), 100);
        //delay(100);
        beep(500);
        return; // Exit function if sensor value too high
      }

      maxSensorValue = sensorValue;
      currentBestGain = gain;
      break; // Exit loop if condition met
    }

    else if (sensorValue > maxSensorValue && sensorValue < 27000) {
      maxSensorValue = sensorValue;
      currentBestGain = gain;
      if (maxSensorValue > 13500) break;
    }

  }

  if (maxSensorValue < 13000) {
    String errorMessage = String(actualChannel) + ": Low Value!";
    Serial.println(errorMessage);
    sendText(diagnosticBox, errorMessage.c_str(), 100);
    beep(500);
    //delay(100);
    return;
  }

  bestGain[ledID] = (currentBestGain - GAIN_ONE) / (GAIN_TWO - GAIN_ONE);
  EEPROM.write(269 + ledID, bestGain[ledID]); //starting address is 269
  EEPROM.commit();

  Serial.println("sensorvalue = " + String(maxSensorValue) + ", bestGain = " + gainMapping[bestGain[ledID]] / 512);
}

void readBestGainFromEEPROM() {
  int address = 269; // Starting memory address

  for (int i = 0; i < 32; i++) {
    bestGain[i] = EEPROM.read(address + i);
  }
}

void determineBestGainForEachLED() {
  for (int ledID = 0; ledID < NUM_LEDS; ledID++) {
    determineBestGainForLED(ledID);
  }
  Serial.println();
}

void testSensor() {
  testSensorFlag = true;
  for (int led = 0; led < 32; led++) {
    sensorRead(led);
  }
  testSensorFlag = false;
  Serial.println();
}
