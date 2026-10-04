/*
  Project:      Climatron - Your personal air quality monitoring robot
  Description:  public (non-secret) configuration data
*/

#pragma once

// Configuration Step 1: Create and/or configure secrets.h. Use secrets_template.h as guide to create secrets.h

// Configuration Step 2: Set network data endpoints
// #define MQTT     // log sensor data to MQTT broker
// #define HASSIO_MQTT  // And, if MQTT enabled, with Home Assistant too?
// #define INFLUX // Log data to InfluxDB server
#define THINGSPEAK  // Log data to ThingSpeak

// Configuration Step 3: Set debug message mode
// Modes
// 0 = OFF
// 1 = summary
// 2 = verbose
#define DEBUG 2

// Configuration Step 4: Set hardware simulation mode
// Modes
// 0 = OFF
// 1 = random values every time
// 2 = random starting values, slightly +/- per cycle
// 3 = out of bounds values every time
// 4 = rapid directional change > rapid-rise threshold for one characteristic
// 5 = small, consistent directional change < rapid-rise threshold for one characteristic
#define HARDWARE_SIMULATE 0
constexpr uint8_t kSimulationCycles = 10;

//////////////////////////////////////
// Below are configuration variables that are less likely to require changes
//////////////////////////////////////

// Open Weather Map (OWM)
const String kOWMServer = "https://api.openweathermap.org/data/2.5/";
const String kOWMAQMPath = "air_pollution?";
const String kOWMForecastPath = "forecast?";
// OWM Air Pollution scale from https://openweathermap.org/api/air-pollution
const String OWMPollutionLabel[5] = {"Good", "Fair", "Moderate", "Poor", "Very Poor"};

// UI
enum screenNames {sMain, sCO2, sPM25, sVOC, sNOX, sForecast};

// screen layout assists in pixels
constexpr uint8_t kXMargins = 5;
constexpr uint8_t kYMargins = 5;
constexpr uint8_t kYStatusRegion = 30; // display.height()/8 assuming 320x240 screen

// background color used for the "TODAY" column in the Weather Forecast screen (see screens.cpp)
#define TFT_TODAYBG 0x41e8

// Display 
constexpr uint8_t screenRotation = 3; // CYD 2.8; horizontal orientation with USB port on left side

// brightness maximum and minimum levels (0-255)
constexpr uint8_t screenBLMax = 255;
constexpr uint8_t screenBLLow  = 52;   // 255 * 0.20

// warnings
const String kWarningLabel[4]={"Good", "Fair", "Poor", "Bad"};
// Subjective color scheme using 16 bit ('565') RGB colors
constexpr uint16_t kWarningColor[4] = {
    0x07E0, // Green = "Good"
    0xFFE0, // Yellow = "Fair"
    0xFD20, // Orange = "Poor"
    0xF800  // Red = "Bad"
  };

// Timers
// Internet and network endpoints
constexpr uint8_t timeConnectTimeoutSeconds = 10; // how long WFM attempts network connect before failing
constexpr uint32_t timeOWMRenewMS = 1800000; // min time between OWM calls
constexpr uint32_t timeWebPortalTimeOutMS = 360000; // how long web configuration portal stays active

constexpr uint32_t timeHardwareSleepTimeμS = 10000000;  // sleep time if hardware error occurs
// button press length
constexpr uint32_t timeStartPortalHoldMS = 5000;  // long-press duration to start config portal
constexpr uint32_t timeDeviceResetHoldMS = 10000; // Long-press duration to wipe config

constexpr uint32_t timeScreenSaverStartMS = 300000; // switch to screen saver if no input after this period

// Sampling and reporting intervals

// How many samples are retained by Measure library in a FIFO queue
constexpr uint8_t kSampleCapacity = 10;

#if defined (DEBUG) && !defined (HARDWARE_SIMULATE)
  // time between sensor reads, e.g. samples
  constexpr uint32_t kTimeSensorSampleMS = 30000; // minimum inter-sample time for many sensors
#elif defined(DEBUG) && defined (HARDWARE_SIMULATE)
  constexpr uint32_t kTimeSensorSampleMS = 15000; // rapid samples for debugging
#else // Production sample pace
  constexpr uint32_t kTimeSensorSampleMS = 60000;
#endif
// time between samplePost()
constexpr uint32_t timeReportMS = kTimeSensorSampleMS * kSampleCapacity;

constexpr uint8_t reportFailureThreshold = 3; // report attempt failures before UI alert starts
constexpr uint8_t kRequiredRisingDeltas = 3; // minimum deltas required to trigger rapid rise alert

// Boundary values

// constexpr uint8_t kOWMAQIMin = 1;  // https://openweathermap.org/api/air-pollution
// constexpr uint8_t kOWMAQIMax = 5;

constexpr uint8_t kNetworkRSSIMin = 30; // ESP32 abs(WiFi.RSSI()) spec
constexpr uint8_t kNetworkRSSIMax = 100;
constexpr uint8_t kNetworkRSSISimVariability = 5; // max RSSI can change per sim cycle

enum kSensorType : uint8_t {
    SENSOR_NONE = 0,
    SENSOR_TEMP,
    SENSOR_HUMIDITY,
    SENSOR_CO2,
    SENSOR_PM25,
    SENSOR_VOC,
    SENSOR_NOX,
    SENSOR_COUNT
};

constexpr float   kSigmaMultiplier = 2.5f;

// tempF value threshholds
constexpr uint16_t kSensorTempFMin =       14; // -10C per SCD40, SEN66 datasheet
constexpr uint8_t kSensorTempFComfortMin = 65;
constexpr uint8_t kSensorTempFComfortMax = 80;
constexpr uint16_t kSensorTempFMax =       122; // 50C per SEN66 datasheet
constexpr uint8_t kSensorTempVariability = 3; // in F

// humidity value thresholds
constexpr uint16_t kSensorHumidityMin =    0; // RH% per datasheet
constexpr uint8_t kSensorHumidityComfortMin = 40;
constexpr uint8_t kSensorHumidityComfortMax = 60;
constexpr uint16_t kSensorHumidityMax =    100;
constexpr uint8_t kSensorHumidityVariability = 3;

// CO2 value thresholds
constexpr uint16_t kSensorCO2Min =   400;   // in ppm
constexpr uint16_t kSensorCO2Fair =  800;
constexpr uint16_t kSensorCO2Poor =  1200;
constexpr uint16_t kSensorCO2Bad =   1600;
constexpr uint16_t kSensorCO2Max =   5000; // SEN6x raw up to 40000
constexpr uint8_t co2SensorReadFailureLimit = 20;
constexpr uint8_t kSensorCO2Variability = 25;

// Particulates (pm1, pm2.5, pm4, pm10) value thresholds
constexpr uint16_t kSensorPMMin =  0;  // per datasheet
constexpr uint16_t kSensorPMFair = 10; // in μg/m3, 2024 EPA breakpoints
constexpr uint16_t kSensorPMPoor = 55;
constexpr uint16_t kSensorPMBad =  125;
constexpr uint16_t kSensorPMMax =  1000; // per SEN54, SEN66 datasheet
constexpr uint8_t kSensorPMVariability = 10;

// VOC (volatile organic compounds) index value thresholds
constexpr uint16_t  kSensorVOCMin =  0;    // per SEN54, SEN66 datasheet
constexpr uint16_t  kSensorVOCFair = 150;
constexpr uint16_t  kSensorVOCPoor = 250;
constexpr uint16_t  kSensorVOCBad =  400;
constexpr uint16_t  kSensorVOCMax =  500;  // per SEN54, SEN66 datasheet
constexpr uint8_t kSensorVOCVariability = 5;

// NOx (nitrogen oxide) index value thresholds, Sensiron Info_Note_NOx_Index.pdf
constexpr uint16_t kSensorNOxMin =   0;    // per SEN66 datasheet
constexpr uint16_t kSensorNOxFair =  49;
constexpr uint16_t kSensorNOxPoor =  150;
constexpr uint16_t kSensorNOxBad =   300;
constexpr uint16_t kSensorNOxMax =   500;  // per SEN66 datasheet
constexpr uint8_t kSensorNOxVariability = 5;

// Physical hardware configuration values
const String hardwareDeviceType = "Climatron";
constexpr uint8_t pinButton = 0; // boot button on most ESP32 boards
constexpr uint8_t pinSensorSDA = 22;
constexpr uint8_t pinSensorSCL = 21;
constexpr uint8_t pinTouchSDA = 33;
constexpr uint8_t pinTouchSCL = 32;
constexpr uint8_t pinTouchRST = 25;
constexpr int8_t pinTouchIRQ = -1;
constexpr uint8_t pinLEDStripOne = 4;
constexpr uint8_t ledStripPixelCount = 3; // number of LEDs on each strip
constexpr int8_t pinAudio = 26;
constexpr uint32_t audioFrequency = 1000; // Hz
constexpr uint8_t  audioResolution = 8;    // bit