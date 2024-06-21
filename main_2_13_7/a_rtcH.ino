#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


WiFiUDP ntpUDP;

const char* ntpServers[] = {
  "time.nist.gov",
  "in.pool.ntp.org",
  "time.google.com",
  "asia.pool.ntp.org",
  "time.windows.com"  
};

NTPClient timeClient(ntpUDP, ntpServers[0]); // Initialize NTPClient with the first server in the array
const char* currentNtpServer = ntpServers[0]; // Initialize currentNtpServer with the first server in the array

uint32_t previousSynctimeMin = 0;

char macStr[13];

RTC_DS1307 rtc; // Create an instance of the RTC_DS1307 class
