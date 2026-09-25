#define RTC_KEY 0xB00B1EE5


void resetRtcTime() {
  time_t currTime;
  if (rtcInit != RTC_KEY) {
    rtcInit = RTC_KEY;
    sysTime.calendar.tm_hour = 0;
    sysTime.calendar.tm_min = 0;
    sysTime.calendar.tm_sec = 0;
    sysTime.calendar.tm_yday = 2026 - 1900;
    sysTime.calendar.tm_mon = 7;
    sysTime.calendar.tm_mday = 29;
    sysTime.calendar.tm_isdst = -1;
    time_t timestamp = mktime(&sysTime.calendar);
    struct timeval tv = {.tv_sec = timestamp, .tv_usec = 0};
    settimeofday(&tv, NULL);
  }
}

void setRtcTime(uint8_t hour, uint8_t min, uint8_t sec, uint8_t day, uint8_t month, uint16_t year) {
  time_t currTime;
  sysTime.calendar.tm_hour = hour;
  sysTime.calendar.tm_min = min;
  sysTime.calendar.tm_sec = sec;
  sysTime.calendar.tm_yday = year - 1900;
  sysTime.calendar.tm_mon = month;
  sysTime.calendar.tm_mday = day;
  sysTime.calendar.tm_isdst = -1;
  time_t timestamp = mktime(&sysTime.calendar);
  struct timeval tv = {.tv_sec = timestamp, .tv_usec = 0};
  settimeofday(&tv, NULL);
}

void checkTime() {
  if (myTimers.clock == 0) {
    uint8_t currDay = sysTime.calendar.tm_mday;
    time_t currTime;
    time(&currTime);
    sysTime.calendar = *localtime(&currTime);
    if (currDay != sysTime.calendar.tm_mday) {
      sysTime.newDay = 1;
    }
    myTimers.clock = 1;
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