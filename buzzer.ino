typedef enum {
  BUZ_TONE_LOW,
  BUZ_TONE_MID,
  BUZ_TONE_HI,
  BUZ_TONE_WAKEUP,
  BUZ_TONE_NOF
}buz_duration_e;

typedef enum {
  BUZ_PULSE_VERYSHORT,
  BUZ_PULSE_SHORT,
  BUZ_PULSE_MID,
  BUZ_PULSE_LONG,
  BUZ_PULSE_WAKEUP,
  BUZ_PULZE_NOF
}buz_tone_e;


void playTone(uint8_t buzTone, uint8_t buzDuration) 
{
  unsigned int selFreq = 0;
  unsigned long selDuration= 0;
  switch(buzTone) {
    case BUZ_TONE_LOW:
      selFreq = 1000;
      break;
    case BUZ_TONE_MID:
      selFreq = 2400;
      break;
    case BUZ_TONE_HI:
      selFreq = 5000;
      break;
    case BUZ_TONE_WAKEUP:
      break;
    default:
      selFreq = 1000;
      break;
  }

  switch(buzDuration) {
    case BUZ_PULSE_VERYSHORT:
      selDuration = 50;
      break;
    case BUZ_PULSE_SHORT:
      selDuration = 250;
      break;
    case BUZ_PULSE_MID:
      selDuration = 750;
      break;
    case BUZ_PULSE_LONG:
      selDuration = 1500;
      break;
    case BUZ_PULSE_WAKEUP:
      break;
    default:
      selDuration = 5000;
      break;
  }
  tone(PIN_BUZZER, selFreq, selDuration);
}

void testBuzzer(void) {
  logPrintf("\n\rTEST BUZZER");

  digitalWrite(PIN_BUZZER, HIGH);
  delay(250);
  digitalWrite(PIN_BUZZER, LOW);
  //tone(PIN_BUZZER, 2000, 500);
  //delay(500);

}