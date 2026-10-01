const float ADC_RESISTOR = ((10.00f + 24.00f) / 24.00f);

void readVBatt() {
  if (myTimers.rst.adc == 0) {
    digitalWrite(PIN_EN_ADCVBAT, HIGH);
    //delay(1);
    //int rawValue = analogRead(PIN_ADC_VBAT);
    float v_adc = analogReadMilliVolts(PIN_ADC_VBAT) / 1000.00f; 
    float v_batt = v_adc * ADC_RESISTOR;
    //logPrintf("\n\rVbatt: %1.2f", v_batt);
    if (v_batt != 0) {  
      ambData.vBatt = v_batt;
    }
    digitalWrite(PIN_EN_ADCVBAT, LOW);
    myTimers.rst.adc = 1;
  }
}