#include <esp_timer.h>
#include "esp_clk_tree.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include <Preferences.h>
#include <time.h>
#include <sys/time.h>
#include <Wire.h>
#include "bsec2.h"

extern "C" {
  #include "soc/rtc.h"
}

const uint8_t currentFwVer = 1;
const uint8_t currentFwRel = 0;
const char* github_user = "pietrotti97";
const char* github_repo = "NotSmartAlarmClock";

#define PIN_BTN1         0
#define PIN_ENCODER_BTN  1
#define PIN_3V3_SW       2
#define PIN_ADC_VBAT     3
#define PIN_ENCODER_S1   20
#define PIN_EN_ADCVBAT   21
#define PIN_ENCODER_S2   10
#define PIN_SWITCH_A     4 
#define PIN_SWITCH_B     5
#define PIN_BACKLIGHT    6
#define PIN_BUZZER       7
#define I2C_SDA          8
#define I2C_SCL          9

#define PRESS_HISTORY_HOURS  12 //12 hours
#define PRESS_FETCH_TIME_MIN 5  // fetch the value every 5 minutes
#define PRESS_BUFFER_SIZE ((PRESS_HISTORY_HOURS * 60) / PRESS_FETCH_TIME_MIN)


#define RTC_KEY 0xB00B1EE5
typedef struct {
  struct {  
    uint32_t magic;
    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
  }bsec2;
  struct {
    uint32_t magic;
    float buf[PRESS_BUFFER_SIZE];
    uint16_t count;
    bool full;
  }forecast;
  struct {
    struct {
      uint8_t status;
      uint8_t result;
      uint8_t timeDelay;
      uint8_t ipAddr[4];
      uint8_t strength;
      uint8_t syncToday;
      tm lastSyncTime;
    } lastConn;
  }wifi;
} BackupDataRTC;

RTC_NOINIT_ATTR uint32_t rtcInit;
RTC_NOINIT_ATTR BackupDataRTC rtcBkp;

Preferences myPreferences;
#define EEPROM_NAMESPACE "tableClock"
#define EEPROM_KEY       "settings"

typedef struct {
  struct tm calendar = {0, 0, 0, 29, 7, 126};
  bool newDay;
}time_s;
time_s sysTime;

#define SYSTEM_SLEEP_WAIT_LONG    600 // 60 seconds before going to sleep if no user button
#define SYSTEM_SLEEP_WAIT_MID     100 // 30 seconds before going to sleep if no user button
#define SYSTEM_SLEEP_WAIT_FAST    5  // 250ms before going to sleep when waking up from sleep cause rtc alarm

//#define SERIAL_ENABLED

typedef struct {
  // perpetual timers
  struct {
    uint8_t weatherDataFetch;
  }noRst;
  // reset these timers on boot
  struct {
    uint8_t uiRefresh;
    uint8_t btnTimerElapsed;
    uint8_t uiTout;
    uint8_t clock;
    uint8_t sensor;
    uint8_t buttons;
    uint16_t system; // timer that decides when system goes to sleep
    uint8_t eeprom;
    uint8_t backlight;
    uint8_t adc;
    uint16_t BME68xMeasTimerElapsed;
  }rst;
}myTimer_s;
volatile myTimer_s myTimers;

typedef struct {
  struct {
    int8_t steps;   
    uint8_t click; 
  }encoder;
  uint8_t sideSwitch; // 0 alarm off, 1 alarm on, 2 alarm snooze
  uint8_t topButton;
}inputs_e;
volatile inputs_e btnStatus;

typedef enum {
  ALARM_OFF,
  ALARM_ON,
  ALARM_SNOOZE
}alarmType_e;
  

typedef enum {
  FORECAST_NONE,                    // dato non disponibile
  FORECAST_STABILE_SERENO,          // Bel tempo, stabile e senza variazioni
  FORECAST_SOLE_SECCO,              // Soleggiato, asciutto, alta pressione
  FORECAST_VARIBILE_MIGLIORAMENTO,  // In miglioramento con schiarite
  FORECAST_NUVOLOSO_STABILE,        // Nuvoloso ma stabile
  FORECAST_NEBBIA_FOSCHIA,          // Possibile nebbia o foschia (alta umidità)
  FORECAST_INSTABILE,               // Poco nuvoloso / Instabilità passeggera
  FORECAST_LENTO_PEGGIORAMENTO,     // Tendenza al lento peggioramento, nuvole
  FORECAST_PIOGGIA_CONTINUA,        // Peggioramento esteso, pioggia diffusa
  FORECAST_PIOGGIA_IMMINENTE,       // Calo rapido, pioggia a breve termine
  FORECAST_TEMPORALE_VENTO,         // Crollo rapido, forte maltempo e vento
  FORECAST_ENUM_NOF
}weatherForecast_e;

typedef enum {
  AIR_UNKNOWN,
  AIR_EXCELLENT,
  AIR_GOOD,
  AIR_FAIR,
  AIR_POOR,
  AIR_BAD,
  AIR_VERY_BAD,
  AIR_NOF
}airQuality_e;

typedef struct {
  struct {
    int16_t secondsDriftPerDay;
    float timezone;
    uint8_t isDST;
  }time;
  struct {
    uint8_t hour;
    uint8_t min;
    uint8_t dow;  // bitmask as xdlmmgvs
    uint8_t snoozeMin;  // snooze minutes
  }alarm;
  struct {
    int16_t altitude;
  }info;
  struct {
    char ssid[33];
    char pwd[64];
    uint8_t syncHour;
  }wifiNet;
  struct {
    uint8_t durationSec;
    uint8_t perc;
    uint8_t stbToutSec;
    uint8_t contrast;
  }backlight;
}data_s;

typedef struct {
  float vBatt;
  weatherForecast_e forecastVal;  // 0 no data, 1 stab
  airQuality_e airQuality;
  struct {
    uint64_t timestamp; // ms
    float temperature;  // C
    float humidity;     // %rh
    float pressure;     // Pa
    float eCO2;         // ppm
    float bVOC;         // ppm
    float IAQ;          // na
    float gasPerc;      // %    
    struct {
      uint8_t stabilize;
      uint8_t runin;
      uint8_t accuracy;
      uint16_t count;
    }status;
    struct {
    float eCO2;         // ppm
    float bVOC;         // ppm
    float IAQ;          // na
    float gasPerc;      // %    
    }debug;
  }bsec2;
}sensor_s;
sensor_s ambData;


typedef struct {
  data_s data;
  uint16_t crc16;
}eepromData_s;
eepromData_s eeprom;

static void IRAM_ATTR TimerCallback(void* arg) {
  static uint16_t counter = 0;
  
  // set here 1ms timer
  readButtons();
  if (myTimers.rst.BME68xMeasTimerElapsed > 0) {myTimers.rst.BME68xMeasTimerElapsed --;}


  if (counter % 10 == 0) {
    // set here 10ms timers
    if (myTimers.rst.btnTimerElapsed > 0) { myTimers.rst.btnTimerElapsed --; }
    if (myTimers.rst.uiRefresh > 0) {myTimers.rst.uiRefresh --;}

    if (counter % 100 == 0) {
      // set here 100ms timers
      if (myTimers.rst.clock > 0) {myTimers.rst.clock --;}
      if (myTimers.rst.system > 0) {myTimers.rst.system --;}

      if (counter % 1000 == 0) {
        // set here 1second timer
        if (myTimers.rst.sensor > 0) {myTimers.rst.sensor --;}
        if (myTimers.rst.uiTout > 0) {myTimers.rst.uiTout --;}
        if (myTimers.rst.eeprom > 0) {myTimers.rst.eeprom --;}  
        if (myTimers.rst.backlight > 0) {myTimers.rst.backlight --;}
        if (myTimers.rst.adc > 0) { myTimers.rst.adc --;}
        if (myTimers.rst.system > 0) {myTimers.rst.system --;}
        //if (myTimers.noRst.weatherDataFetch > 0) {myTimers.noRst.weatherDataFetch --;}
        counter = 0;
      }
    }
  }  
  counter ++;
  if (counter >= 1000) {
    counter = 0;
  }
}

void initPins(void) 
{
  pinMode(PIN_3V3_SW, OUTPUT);
  digitalWrite(PIN_3V3_SW, LOW);

  pinMode(PIN_BTN1, INPUT);
  pinMode(PIN_ENCODER_BTN, INPUT);
  pinMode(PIN_ENCODER_S1, INPUT);
  pinMode(PIN_ENCODER_S2, INPUT);
  pinMode(PIN_SWITCH_A, INPUT);
  pinMode(PIN_SWITCH_B, INPUT);

  pinMode(PIN_EN_ADCVBAT, OUTPUT);
  pinMode(PIN_ADC_VBAT, ANALOG);
  analogReadResolution(12); 
  analogSetPinAttenuation(PIN_ADC_VBAT, ADC_11db); 
  
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  //pinMode(PIN_BACKLIGHT, OUTPUT);
  ledcAttach(PIN_BACKLIGHT, 5000, 8);  // 5khz 8bit
  ledcWrite(PIN_BACKLIGHT, 255);
  //testBuzzer();
}

void deInitPins(void)
{
  digitalWrite(PIN_3V3_SW, HIGH);
  digitalWrite(PIN_BACKLIGHT, HIGH);
  digitalWrite(PIN_BUZZER, LOW);
}

void initSerial(void)
{
  Serial.begin(9600);
  Serial.println("========== SETUP ==========");
}

void initI2C(void)
{
  Wire.begin(I2C_SDA, I2C_SCL);
}

void deInitI2C(void) 
{
  
}

void getWakeupCause(void)
{
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    myTimers.rst.system = SYSTEM_SLEEP_WAIT_FAST;
  } else if (cause == ESP_SLEEP_WAKEUP_GPIO) {
    myTimers.rst.system = SYSTEM_SLEEP_WAIT_MID;
  } else {
    myTimers.rst.system = SYSTEM_SLEEP_WAIT_FAST;
  }
}
void setup() {
  // put your setup code here, to run once:  
  initPins();

  initI2C();
  initLCD();
  initTimer();  
  delay(1);
}

void loop() {
  memset((void*)&myTimers.rst, 0, sizeof(myTimers.rst));
  //initSensorFunc();

  getWakeupCause();
#ifdef SERIAL_ENABLED
  initSerial();
#endif

  // all init here
  startTimer();
  while (1) {
    manageEEProm();
    checkVariables();
    checkTime();
    checkSyncTimeNetwork();
    wifiHandler();
    newDayStuff();
    readVBatt();
    readSensorFunc();
    checkAlarm();
    playAlarmMusic();
    manageBacklight();
    calcWeather();
    calcAirQuality();
    interface();
#ifndef SERIAL_ENABLED
    if (myTimers.rst.system == 0 && interfaceCanSleep() == 1 && alarmRinging() == 0) { break; }
#endif
  }
  backlightOff();
  stopTimer();
  //deInitPins();
  gotoLightSleep();
}

extern "C" void esp_clk_slowclk_cal_set(uint32_t cal_val);

void gotoLightSleep() {
  gpio_wakeup_enable((gpio_num_t)PIN_BTN1, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_ENCODER_BTN, GPIO_INTR_LOW_LEVEL);
  switch (btnStatus.sideSwitch) {
    default:
    case 0:
      gpio_wakeup_enable((gpio_num_t)PIN_SWITCH_A, GPIO_INTR_LOW_LEVEL);
      gpio_wakeup_enable((gpio_num_t)PIN_SWITCH_B, GPIO_INTR_LOW_LEVEL);
    break;

    case 1:
      gpio_wakeup_enable((gpio_num_t)PIN_SWITCH_A, GPIO_INTR_HIGH_LEVEL);
      gpio_wakeup_enable((gpio_num_t)PIN_SWITCH_B, GPIO_INTR_LOW_LEVEL);
    break;

    case 2:
      gpio_wakeup_enable((gpio_num_t)PIN_SWITCH_A, GPIO_INTR_LOW_LEVEL);
      gpio_wakeup_enable((gpio_num_t)PIN_SWITCH_B, GPIO_INTR_HIGH_LEVEL);
    break;
  }
  gpio_hold_en((gpio_num_t)PIN_3V3_SW);
  gpio_hold_en((gpio_num_t)PIN_EN_ADCVBAT);
  gpio_hold_en((gpio_num_t)PIN_BACKLIGHT);

  //gpio_sleep_sel_dis((gpio_num_t)I2C_SDA);
  //gpio_sleep_sel_dis((gpio_num_t)I2C_SCL); 
  //gpio_pullup_en((gpio_num_t)I2C_SDA);
  //gpio_pullup_en((gpio_num_t)I2C_SCL);

  gpio_hold_en((gpio_num_t)I2C_SDA);
  gpio_hold_en((gpio_num_t)I2C_SCL);

  esp_sleep_enable_gpio_wakeup();

  struct timeval now;
  gettimeofday(&now, nullptr);
  const uint64_t US_PER_SECOND = 1000000ULL;
  const uint64_t US_PER_MINUTE = 60ULL * US_PER_SECOND;

  uint64_t nowUs = ((uint64_t)now.tv_sec * US_PER_SECOND) + (uint64_t)now.tv_usec;
  //uint64_t nextMinuteUs = ((nowUs / US_PER_MINUTE) + 1ULL) * US_PER_MINUTE;
  uint64_t nextMinuteUs = (((nowUs / US_PER_MINUTE) + 1ULL) * US_PER_MINUTE) + 500000ULL;
  uint64_t sleepDuration = nextMinuteUs - nowUs;

  if (sleepDuration < 10000ULL) { sleepDuration = 10000ULL; }
  esp_sleep_enable_timer_wakeup(sleepDuration);
  
  esp_light_sleep_start();

  struct timeval afterSleep;
  gettimeofday(&afterSleep, nullptr);
  uint64_t actualSleepUs = ((uint64_t)afterSleep.tv_sec * US_PER_SECOND + afterSleep.tv_usec) - nowUs;
  int64_t driftUs = ((int64_t)actualSleepUs * eeprom.data.time.secondsDriftPerDay) / 86400LL;
  int64_t correctedTimeUs = ((int64_t)afterSleep.tv_sec * US_PER_SECOND) + afterSleep.tv_usec + driftUs;

  afterSleep.tv_sec = correctedTimeUs / US_PER_SECOND;
  afterSleep.tv_usec = correctedTimeUs % US_PER_SECOND;

  settimeofday(&afterSleep, nullptr);

  gpio_hold_dis((gpio_num_t)PIN_3V3_SW);
  gpio_hold_dis((gpio_num_t)PIN_EN_ADCVBAT);
  gpio_hold_dis((gpio_num_t)PIN_BACKLIGHT);
  gpio_hold_dis((gpio_num_t)I2C_SDA);
  gpio_hold_dis((gpio_num_t)I2C_SCL);
}
