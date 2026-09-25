

void readButtons(void)
{
  checkEncoder();
  checkSideSwitch();
  checkTopButton();
}

void checkEncoder() {
  static uint8_t prevAB = 0;
  static int8_t encState = 0;

  static uint8_t btnHistory = 0xFF;
  static uint8_t lastBtnStable = 1;

  uint8_t a = digitalRead(PIN_ENCODER_S1);
  uint8_t b = digitalRead(PIN_ENCODER_S2);
  uint8_t ab = (a << 1) | b;

  uint8_t index = (prevAB << 2) | ab;

  static const int8_t table[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0
  };

  int8_t movement = table[index];

  encState += movement;

  // IMPORTANT: convert transitions → 1 detent
  if (encState >= 4) {
    btnStatus.encoder.steps++;
    myTimers.system = SYSTEM_SLEEP_WAIT_MID;
    encState = 0;
  } else if (encState <= -4) {
    btnStatus.encoder.steps--;
    myTimers.system = SYSTEM_SLEEP_WAIT_MID;
    encState = 0;
  }
  prevAB = ab;

  uint8_t rawBtn = digitalRead(PIN_ENCODER_BTN);
  btnHistory = (btnHistory << 1) | rawBtn;
  if (btnHistory == 0x00 && lastBtnStable == 1) {
    btnStatus.encoder.click = 1;
    myTimers.system = SYSTEM_SLEEP_WAIT_MID;
    lastBtnStable = 0;
  } else if (btnHistory == 0xFF && lastBtnStable == 0) {
    lastBtnStable = 1;
  }
}

void checkSideSwitch(void)
{
  static uint8_t stable_A = 1;
  static uint8_t stable_B = 1;
  static uint8_t btnHistory_A = 0xFF;
  static uint8_t btnHistory_B = 0xFF;
  uint8_t rawBtn_A = digitalRead(PIN_SWITCH_A);
  uint8_t rawBtn_B = digitalRead(PIN_SWITCH_B);
  //logPrintf("\n\rBTN A: %d, BTN B: %d", rawBtn_A, rawBtn_B);
  btnHistory_A = (btnHistory_A << 1) | rawBtn_A;
  btnHistory_B = (btnHistory_B << 1) | rawBtn_B; 
  bool stateChanged = false;

  if (btnHistory_A == 0x00 && stable_A == 1) {
    stable_A = 0;
    stateChanged = true;
  } else if (btnHistory_A == 0xFF && stable_A == 0) {
    stable_A = 1;
    stateChanged = true;
  }

  if (btnHistory_B == 0x00 && stable_B == 1) {
    stable_B = 0;
    stateChanged = true;
  } else if (btnHistory_B == 0xFF && stable_B == 0) {
    stable_B = 1;
    stateChanged = true;
  }
  if (stateChanged) {
    if (stable_A == 0) {
      btnStatus.sideSwitch = 1;
      logPrintf("\n\rPosition 1");
    } else if (stable_B == 0) {
      btnStatus.sideSwitch = 2;
      logPrintf("\n\rPosition 2");
    } else {
      btnStatus.sideSwitch = 0;
      logPrintf("\n\rPosition 0");
    }
  }
}

void checkTopButton(void)
{
  static uint8_t btnHistory = 0xFF;
  static uint8_t lastBtnStable = 1;
  uint8_t rawBtn = digitalRead(PIN_BTN1);
  btnHistory = (btnHistory << 1) | rawBtn;
  if (btnHistory == 0x00 && lastBtnStable == 1) {
    btnStatus.topButton = 1;
    myTimers.system = SYSTEM_SLEEP_WAIT_MID;
    lastBtnStable = 0;
  } else if (btnHistory == 0xFF && lastBtnStable == 0) {
    lastBtnStable = 1;
  }
}