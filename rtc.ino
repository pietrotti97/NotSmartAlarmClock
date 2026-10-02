void resetRtcTime() {
  if (rtcInit != RTC_KEY) {
    rtcInit = RTC_KEY;
    struct tm t;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_mday = 29;
    t.tm_mon = 8 - 1;
    t.tm_year = 2026 - 1900;
    t.tm_isdst = -1;
    time_t timestamp = mktime(&t);
    struct timeval tv = {.tv_sec = timestamp, .tv_usec = 0};
    settimeofday(&tv, NULL);
  }
}


void setRtcTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t day, uint8_t month, uint16_t year) {
  struct tm t;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  t.tm_mday = day;
  t.tm_mon = month -1;
  t.tm_year = year - 1900;
  t.tm_isdst = -1;
  time_t timestamp = mktime(&t);
  struct timeval tv = {.tv_sec = timestamp, .tv_usec = 0};
  settimeofday(&tv, NULL);
}

void checkTime() {
  if (myTimers.rst.clock == 0) {
    uint8_t currDay = sysTime.calendar.tm_mday;
    time_t currTime;
    time(&currTime);
    sysTime.calendar = *localtime(&currTime);
    if (currDay != sysTime.calendar.tm_mday) {
      sysTime.newDay = 1;
    }
    myTimers.rst.clock = 1;
  }
}

void newDayStuff() {
  if (sysTime.newDay == 1) {

    sysTime.newDay = 0;
  }
}
/*
void getRTCCalibrationInfo(void)
{
  logPrintf("Before:");
  logPrintf(rtc_clk_slow_freq_get());

  rtc_clk_slow_freq_set(RTC_SLOW_FREQ_8MD256);

  logPrintf("After:");
  logPrintf(rtc_clk_slow_freq_get());

uint32_t cal = rtc_clk_cal(RTC_CAL_RTC_MUX, 1000);

double period_us = (double)cal / (1UL << 19);
double frequency_hz = 1000000.0 / period_us;

logPrintf("RTC source = %d\n", rtc_clk_slow_freq_get());
logPrintf("RTC cal    = %u\n", cal);
logPrintf("RTC period = %.6f us\n", period_us);
logPrintf("RTC freq   = %.3f Hz\n", frequency_hz);

}*/