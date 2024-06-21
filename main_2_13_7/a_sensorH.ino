
#include <Adafruit_ADS1X15.h>
#include <math.h>

//------------------------- pin declaration start--------------------------//
// Photodiode Pins
#define SDA_PHOTODIODE 21
#define SCL_PHOTODIODE 22

// Heater & Temperature Sensor Pins
#define HEATER_PIN 2
#define SDA_TEMP 33
#define SCL_TEMP 32

// Stepper Motor Pins
#define STEP_PIN 18
#define DIR_PIN 19
#define MOT_DRIVER_PIN 25
#define LIMIT_PIN 15

// Mux Control Pins
int muxS0 = 4;
int muxS1 = 12;
int muxS2 = 13;
int muxS3 = 14;

// Mux "SIG" Pins
int COM1 = 27;
int COM2 = 26;

//------------------------- pin declaration ends --------------------------//

// Create instances of the Adafruit ADS1115 class for each device
Adafruit_ADS1115 ads1; //0X49
Adafruit_ADS1115 ads2; //0X4B
Adafruit_ADS1115 ads3; //0x48
Adafruit_ADS1115 ads4; //0X4A
Adafruit_ADS1115 adsTemp; //0X48

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

// ADS1115 declaration ends

bool whiteAllCommandSent;

bool testingPhotodiode = false; //true when testing photodiodes. true will turn off the LEDs.

String state;

long firstReading[numChannels*2];

int progressValues[numChannels];

//int whiteIntensity[numChannels*2];
uint16_t whiteIntensity[numChannels*2]={23154,11073,25107,13987,23393,11961,24426,11920,9913,9914,20386,9660,20046,9768,19143,9810};
uint16_t latestIntensity[numChannels*2]={};

int channelarrangement[numChannels*2] = {6,7,4,5,2,3,0,1,14,15,12,13,10,11,8,9,24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23};


// setting PWM properties
const int freq = 5000;
const int pwmChannel = 1;
const int ledpwmChannel = 2;
const int resolution = 10;

int current_limitpin, prev_limitpin;


#define VCC 3.29    //Supply   voltage
#define ADCmax 26287 //TempADC maxvalue at VCC input

float temperature;
float newTemp = 37.6; // temperature shift value
float setTemp = newTemp; //deg C
float MinTemp = setTemp - 5; // [degC] minimum expected temperature (needed for rescaling inputs)
float MaxTemp = setTemp; // [degC] maximum allowed temperature, over which heater is turned off (needed for rescaling inputs)
float tempDiff = 0;

const int dt = 500; // [ms] time constant in milliseconds(controller clock rate = 1/(dt/1000) [Hz])
int SetTime = 1800; // [s] timer in seconds, if reached, running stops [Default: 1800]  
double K_P_ctrl = 10; //proportional gain
double K_I_ctrl = 0; //integral gain (set to lower values i.e. 10^-3)
double K_D_ctrl = 0; //derivative gain
///////////////////////////////////
#define Rt0 100000   // O
#define Ra 100000  //Ra=100KO
#define Ba 3950     //   K

float RT, VR, ln, tx, t0, VRT, previous_error, s_integral;
bool bInRange = 0;
int TicksPerMS = floor(1000/dt);

int progressThreshold = 65;
const float datainterval = 0.5; //in min
uint16_t progressHistory[16][3];
uint16_t progressAvg[16];
uint16_t lastProgressAvg[16];

uint32_t lastDisplayTime = 0;
char userEmail[50];
char userPassword[50];

uint16_t first1;
uint16_t first2;

uint8_t channelCompleteCount[16];

uint8_t channelReadCount[numChannels] = {0};

#define NUM_LEDS numChannels*2
adsGain_t gainMapping[] = {GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT}; // Define a mapping between int and adsGain_t

uint8_t bestGain[NUM_LEDS];

bool testSensorFlag = false;
bool logflag = false;
int minContrast = 600;
