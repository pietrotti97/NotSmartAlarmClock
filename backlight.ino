#define BKL_PWM_LO  230
#define BKL_PWM_MID 128
#define BKL_PWM_HI  20

typedef struct {
  uint8_t alwaysOn;
}dataBacklight_s;

dataBacklight_s dataBkl;

void setBacklight(bool state, uint8_t perc, uint8_t alwaysOn) {
  if (state == true) {
    int pwmVal = map(perc, 0, 100, 255, 0);
    ledcWrite(PIN_BACKLIGHT, pwmVal);
    myTimers.backlight = eeprom.data.backlight.durationSec;
    dataBkl.alwaysOn = alwaysOn;
  } else {
    ledcWrite(PIN_BACKLIGHT, 255);
    dataBkl.alwaysOn = 0;
  }
}

void backlightOn(void)
{
  eeprom.data.backlight.perc = constrain(eeprom.data.backlight.perc, 0, 100);
  int pwmVal = map(eeprom.data.backlight.perc, 0, 100, 255, 0);
  myTimers.backlight = eeprom.data.backlight.durationSec;
  ledcWrite(PIN_BACKLIGHT, pwmVal);
}

void backlightOff(void)
{
  dataBkl.alwaysOn = 0;
  ledcWrite(PIN_BACKLIGHT, 255);
}

void manageBacklight(void) {
  if(dataBkl.alwaysOn == 0 && myTimers.backlight == 0) {
    ledcWrite(PIN_BACKLIGHT, 255);
  }
}