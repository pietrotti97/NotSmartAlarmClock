void manageEEProm(void) {
  static bool eepFirstRead = false;
  //if ( datasize >= storage.size()) return;  // it's 1020
  if (!eepFirstRead) {
    eepFirstRead = true;
    memset(&eeprom, 0, sizeof(eepromData_s));
    myPreferences.begin(EEPROM_KEY);
    size_t storedSize = myPreferences.getBytesLength(EEPROM_KEY);
    if (storedSize == sizeof(eepromData_s)) {
      myPreferences.getBytes(EEPROM_KEY, &eeprom, sizeof(eepromData_s));
      myPreferences.end();
      logPrintln("Read all structure from eeprom. CRC: ");
      logPrintln(eeprom.crc16);
      uint16_t firstCRC = crc16_update((uint8_t *)&eeprom.data, sizeof(data_s));
      logPrintln("Calculated CRC: ");
      logPrintln(firstCRC);
      if (firstCRC != eeprom.crc16) {
        memset(&eeprom, 0, sizeof(eepromData_s));
      }
    } else {
      memset(&eeprom, 0, sizeof(eepromData_s));
    } 
  }
  if (myTimers.eeprom == 0) {
    uint16_t currCRC = crc16_update((uint8_t *)&eeprom.data, sizeof(data_s));
    if (currCRC != eeprom.crc16) {
      logPrintln("CurrCRC: ");
      logPrintln(currCRC);
      logPrintln(" - CRC NOT MATCHING, writing");
      logPrintln(eeprom.crc16);
      eeprom.crc16 = currCRC;
      myPreferences.begin(EEPROM_KEY);
      myPreferences.putBytes(EEPROM_KEY, &eeprom, sizeof(eepromData_s));
      myPreferences.end();
    }
    myTimers.eeprom = 5;
  }
}