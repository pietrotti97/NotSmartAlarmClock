//min frequency for buzzer is 200MHz
//max frequency for buzzer is 2100 MHz

const uint16_t melody[] = {1000, 1200, 1000, 1500, 1200, 1000};
const uint16_t melodyDuration[] = {150, 150, 150, 300, 150, 400};
const uint8_t melodyLength = sizeof(melody) / sizeof(melody[0]);

typedef struct {
  uint8_t fired;
  uint8_t playMelody;
  uint8_t inSnooze;
  uint32_t nextSnoozeTime;
}alarm_s;
alarm_s tAlarm;

void playAlarmMusic(void) {
  static uint8_t note = 0;
  static unsigned long previousMillis = 0;
  static bool playing = false;

   if (!tAlarm.playMelody) {
     if (playing) {
      noTone(PIN_BUZZER);
      playing = false;
      note = 0;
    }
    return;
  }

  unsigned long now = millis();
  if (!playing) {
    playing = true;
    note = 0;
    previousMillis = now;
    tone(PIN_BUZZER, melody[note]);
    return;
  }

  if (now - previousMillis >= melodyDuration[note]) {
    previousMillis = now;
    note++;
    if (note >= melodyLength) {
      noTone(PIN_BUZZER);
      playing = false;
      // If you want the melody to play only once:
      //playMelody = false;
      return;
    }
    tone(PIN_BUZZER, melody[note]);
  }
}

void checkAlarm(void) 
{
  uint32_t timeNow = (uint32_t)time(nullptr);

  if (tAlarm.fired && sysTime.calendar.tm_min != eeprom.data.alarm.min && !tAlarm.inSnooze) {
    logPrintf("\n\rFired, not snooze, minute is passed. STOP!");
    tAlarm.fired = false; 
  }

  if (!tAlarm.fired && !tAlarm.inSnooze &&
          sysTime.calendar.tm_hour == eeprom.data.alarm.hour &&
            sysTime.calendar.tm_min == eeprom.data.alarm.min &&
              (eeprom.data.alarm.dow & (1 << sysTime.calendar.tm_wday)) )      
  {
    //logPrintf("\n\rNot fired, not snooze, minute match, PLAY!");

    if (btnStatus.sideSwitch == 1 || btnStatus.sideSwitch == 2) {
      tAlarm.fired = true;
      tAlarm.playMelody = true;
    }
  }

  if (tAlarm.inSnooze) {
    //logPrintf("\n\rInSnooze, ");
    if (btnStatus.sideSwitch == 2) {
      //logPrintf("CurrTime: %d", timeNow);
      if (timeNow >= tAlarm.nextSnoozeTime) {
        //logPrintf(", PLAY AGAIN!");
        tAlarm.inSnooze = false;
        tAlarm.fired = true;
        tAlarm.playMelody = true;
      }
    } else {
      tAlarm.inSnooze = false;
    }
  }

  if (btnStatus.sideSwitch == 0) {
    tAlarm.fired = false;
    tAlarm.playMelody = false;
    tAlarm.inSnooze = false;
    tAlarm.nextSnoozeTime = 0;
  }
}

uint8_t alarmRinging(void) 
{
  if (tAlarm.fired == true || tAlarm.playMelody == true || tAlarm.inSnooze == true) 
    return true;
  else
    return false;
}

void playAlarm(void) 
{
  tAlarm.fired = true;
  tAlarm.playMelody = true;
}

void stopAlarm(void) 
{
  //logPrintf("\n\rSTOP!");
  if (tAlarm.playMelody == true) {
    tAlarm.playMelody = false;
    // if Snooze, start 9minutes timer
    if (btnStatus.sideSwitch == 2) {
      uint32_t timeNow = (uint32_t)time(nullptr);
      tAlarm.inSnooze = true;
      tAlarm.nextSnoozeTime = timeNow + (60 * eeprom.data.alarm.snoozeMin);
      logPrintf("\n\rNextAlarm: %d", tAlarm.nextSnoozeTime);
    }
  }
}
