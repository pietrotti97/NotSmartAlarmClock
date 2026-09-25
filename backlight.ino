#define BKL_PWM_LO  230
#define BKL_PWM_MID 128
#define BKL_PWM_HI  20

void setBacklight(bool state) {
  if (state == true) {
    state = false;
    myTimers.backlight = 10;
    ledcWrite(PIN_BACKLIGHT, BKL_PWM_LO);

  }
}

void manageBacklight(void) {
  if(myTimers.backlight == 0) {
    ledcWrite(PIN_BACKLIGHT, 255);
  }
}