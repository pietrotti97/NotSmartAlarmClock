#include <U8g2lib.h>

#define BTN_RELEASED 0
#define BTN_PRESSED  1

U8G2_ST7567_ENH_DG128064I_F_HW_I2C u8g2(U8G2_R2, I2C_SCL, I2C_SDA, U8X8_PIN_NONE); //U8G2_ST7567_JLX12864_F_HW_I2C

typedef enum{
  UI_HOME,              // home screen
  UI_MENU,              // the main menu screen with options
  UI_SET_TIME,          // set time screen
  UI_SET_ALARM,         // set alarm screen
  UI_SET_ELEVATION,     // set elevation screen
  UI_SET_SNOOZETIME,    // set elevation screen
  UI_SET_WIFI,          // set elevation screen
  UI_SET_DUMMY,         // set elevation screen
  UI_SET_SECONDSDRIFT,  // set drift per day in seconds
  UI_SET_BACKLIGHT,     // set backlight intensity, timeout and system sleep
  UI_SET_REALTIMEDATA,  // view realtime data from sensors
  UI_NOF
}uiStates_e;

const char* options[] = { "Set Time", "Set Alarm", "Altitude", "Back", "Snooze Time", "WiFi", "Inculati", "Set Drift/day", "Backlight", "RealTime data"};
const uint8_t optionCount = UI_NOF -1;

typedef struct {
  uiStates_e position;
  bool refresh;

  struct {
    uint8_t index;
    uint8_t offset;
  }menu;
  
  struct {
    uint8_t topBtn;
    uint8_t sideSwitch;
    uint8_t encBtn;
    int8_t encSteps;
  }action;
} uiVars_s;
uiVars_s ui;

#define LCD_UI_TOUT_SHORT 20
#define LCD_UI_TOUT_LONG  60


void initLCD(void)
{
  u8g2.setI2CAddress(0x3F * 2); 
  //u8g2.setBusClock(400000);
  u8g2.begin();  
  u8g2.enableUTF8Print();

  u8g2.clearBuffer();  
  u8g2.setFontMode(1);

  drawString(4, 38, 26, "!CLOCK!");
  u8g2.sendBuffer();
}

bool interfaceCanSleep() {
  if(ui.position == UI_HOME && ui.refresh == false)
    return 1; // if system can go to sleep
  else
    return 0; // if system cannot enter sleep
}


void drawString(uint8_t x, uint8_t y, uint8_t size, const char *s)
{
  if (x>128 || y>64) return;

  switch(size) {
    case 4:
      u8g2.setFont(u8g2_font_4x6_mf);
      break;
    case 5:
      u8g2.setFont(u8g2_font_5x8_mf);
      break;
    case 6:
      u8g2.setFont(u8g2_font_6x12_mf);
      break;    
    case 8:
      u8g2.setFont(u8g2_font_8x13_mf);
      break;
    case 26:
      u8g2.setFont(u8g2_font_logisoso26_tf);
      break;
    case 32:
      u8g2.setFont(u8g2_font_logisoso32_tf);
      break;
    
    default:
      return;
  }
  
  u8g2.setCursor(x, y);
  u8g2.print(s);
}



/*
  Draw a string with specified pixel offset. 
  The offset can be negative.
  Limitation: The monochrome font with 8 pixel per glyph
*/
void drawScrollString(int16_t offset, const char *s)
{
  static char buf[36];	// should for screen with up to 256 pixel width 
  size_t len;
  size_t char_offset = 0;
  u8g2_uint_t dx = 0;
  size_t visible = 0;
  len = strlen(s);
  if ( offset < 0 )
  {
    char_offset = (-offset)/8;
    dx = offset + char_offset*8;
    if ( char_offset >= u8g2.getDisplayWidth()/8 )
      return;
    visible = u8g2.getDisplayWidth()/8-char_offset+1;
    strncpy(buf, s, visible);
    buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(char_offset*8-dx, 62, buf);
  }
  else
  {
    char_offset = offset / 8;
    if ( char_offset >= len )
      return;	// nothing visible
    dx = offset - char_offset*8;
    visible = len - char_offset;
    if ( visible > u8g2.getDisplayWidth()/8+1 )
      visible = u8g2.getDisplayWidth()/8+1;
    strncpy(buf, s+char_offset, visible);
    buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(-dx, 62, buf);
  }
  
}

void draw(const char *s, uint8_t symbol, int degree)
{
  int16_t offset = -(int16_t)u8g2.getDisplayWidth();
  int16_t len = strlen(s);
  for(;;)
  {
    u8g2.firstPage();
    do {
      //drawWeather(symbol, degree);
      //drawString("CIAO!");
      drawScrollString(offset, s);
    } while ( u8g2.nextPage() );
    delay(20);
    offset+=2;
    if ( offset > len*8+1 )
      break;
  }
}

void interface() 
{
  if (myTimers.rst.btnTimerElapsed == 0) {
    if (btnStatus.topButton == 1) {
      logPrintf("\n\rTop Btn click");
      ui.action.topBtn = 1;
      btnStatus.topButton = 0;
    }
    
    ui.action.sideSwitch = btnStatus.sideSwitch;
    //logPrintf("\n\rSideSwitch: %d", ui.action.sideSwitch);

    
    if (btnStatus.encoder.click == 1) {
      logPrintf("\n\rencoder click");
      ui.action.encBtn = 1;
      btnStatus.encoder.click = 0;
    }
   
    if (btnStatus.encoder.steps) {
      ui.action.encSteps = btnStatus.encoder.steps;    
      logPrintf("\n\rencoder Steps: %d", ui.action.encSteps);
      btnStatus.encoder.steps = 0;
    }
    myTimers.rst.btnTimerElapsed = 1;
  }

  if (myTimers.rst.uiRefresh == 0) {
    u8g2.clearBuffer();  
    u8g2.setFontMode(1);
    switch(ui.position) {
      case UI_HOME: displayMain(); break;
      case UI_MENU: displayMenu(); break;
      case UI_SET_TIME: displaySetTime(); break;
      case UI_SET_ALARM: displaySetAlarm(); break;
      case UI_SET_ELEVATION: displaySetElevation(); break;
      case UI_SET_SNOOZETIME: displaySetSnoozetime(); break;
      case UI_SET_WIFI: displaySetWiFi(); break;
      case UI_SET_DUMMY: displaySetDummy(); break;
      case UI_SET_SECONDSDRIFT: displaySetSecondsdrift(); break;
      case UI_SET_BACKLIGHT: displaySetBacklight(); break;
      case UI_SET_REALTIMEDATA: displayRealtimeData(); break;
      default: ui.position = UI_HOME; break;
    }
    u8g2.sendBuffer();
    if (ui.position == UI_HOME) {
      myTimers.rst.uiRefresh = 5;
    } else {
      if (myTimers.rst.uiTout == 0) {
        ui.position = UI_HOME;
        ui.refresh = true;
      }
      if(ui.refresh == 1) {
        ui.refresh = true;
        myTimers.rst.uiRefresh = 0;
      } else {
        myTimers.rst.uiRefresh = 1;
      }
    }
  }
}

const char* wDays[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};
const char* wDaysShort[] = {"D", "L", "M", "M", "G", "V", "S"};


void displayMain(void) 
{
  char string[30];
  snprintf(string, sizeof(string), "%02d:%02d.%02d", sysTime.calendar.tm_hour, sysTime.calendar.tm_min, sysTime.calendar.tm_sec);
  drawString(4, 38, 26, string);
  snprintf(string, sizeof(string), "%s %2d/%02d/%04d", wDays[sysTime.calendar.tm_wday], sysTime.calendar.tm_mday, sysTime.calendar.tm_mon + 1, sysTime.calendar.tm_year + 1900);
  drawString(2, 10, 8, string);

  snprintf(string, sizeof(string), "%2.1f°C", ambData.bsec2.temperature);
  drawString(1, 50, 8, string);
  
  snprintf(string, sizeof(string), "%2.0f%%", ambData.bsec2.humidity);
  drawString(1, 63, 8, string);

  const unsigned char* chosenIco = NULL;
  switch(ambData.forecastVal) {
    case FORECAST_STABILE_SERENO: chosenIco = imgicons8_sole_16; break;                       // Bel tempo, stabile e senza variazioni
    case FORECAST_SOLE_SECCO: chosenIco = imgicons8_estate_16; break;                         // Soleggiato, asciutto, alta pressione
    case FORECAST_VARIBILE_MIGLIORAMENTO: chosenIco = imgicons8_partly_cloudy_day_16; break;  // In miglioramento con schiarite
    case FORECAST_NUVOLOSO_STABILE: chosenIco = imgicons8_nuvola_tratteggiata_16; break;      // Nuvoloso ma stabile
    
    case FORECAST_NEBBIA_FOSCHIA:                                                             // Possibile nebbia o foschia (alta umidità)
      if ((sysTime.calendar.tm_hour >=6) && (sysTime.calendar.tm_hour <= 21))
        chosenIco = imgicons8_giorno_nebbioso_16;
        else
          chosenIco = imgicons8_notte_nebbiosa_16;
      break;
    case FORECAST_INSTABILE: chosenIco = imgicons8_sun_rain_cloud_16; break;                  // Poco nuvoloso / Instabilità passeggera
    case FORECAST_LENTO_PEGGIORAMENTO: chosenIco = imgicons8_cloud_16; break;                 // Tendenza al lento peggioramento, nuvole
    case FORECAST_PIOGGIA_CONTINUA: chosenIco = imgicons8_heavy_rain_16; break;               // Peggioramento esteso, pioggia diffusa
    case FORECAST_PIOGGIA_IMMINENTE: chosenIco = imgicons8_light_rain_16; break;              // Calo rapido, pioggia a breve termine
    case FORECAST_TEMPORALE_VENTO: chosenIco = imgicons8_cloud_lightning_16; break;           // Crollo rapido, forte maltempo e vento
    default:                                                                                  // not enough data
    case FORECAST_NONE:
    case FORECAST_ENUM_NOF:
    chosenIco = imgicons8_sync_16;
    break;
  }
  u8g2.drawXBMP(60, 39, 16, 16, chosenIco);
  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    logPrintf("Home - TopBtn Pressed");
    playTone(BUZ_TONE_LOW, BUZ_PULSE_VERYSHORT);
    backlightOn();
    stopAlarm();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 

  if (ui.action.encBtn == BTN_PRESSED) { 
    ui.position = UI_MENU;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    myTimers.rst.uiRefresh = 0;
    myTimers.rst.uiTout = LCD_UI_TOUT_SHORT;
  }

  if (ui.action.encSteps != 0) {
    ui.action.encSteps = 0;
  }

  switch (ui.action.sideSwitch) {
    case 1:
      // simple alarm
       u8g2.drawXBMP(112, 39, 16, 16, icona_campana);
      break;
    case 2:
      // snooze alarm
      u8g2.drawXBMP(112, 39, 16, 16, icona_snooze);
      break;
    case 0:
    default:
      // alarm off
      u8g2.drawXBMP(112, 39, 16, 16, icona_divieto);
      break;
  }

  if (ambData.vBatt >= 3.80f) {
    u8g2.drawXBMP(112, 57, 16, 8, icona_batteria_carica_16x8);
  } else if ((ambData.vBatt < 3.80f) && (ambData.vBatt >= 3.45f)) {
    u8g2.drawXBMP(112, 57, 16, 8, icona_batteria_meta_16x8);
  } else {
    u8g2.drawXBMP(112, 57, 16, 8, icona_batteria_scarica_16x8);
  }
}

void displayMenu(void) {
  char string[30];
  snprintf(string, sizeof(string), "MENU");
  drawString(50, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);
  int maxVisible = 4;

  if (ui.menu.index >= ui.menu.offset + maxVisible) {
    ui.menu.offset = ui.menu.index - maxVisible + 1;
  }
  if (ui.menu.index < ui.menu.offset) {
    ui.menu.offset = ui.menu.index;
  }

  for (int i = 0; i < maxVisible && (ui.menu.offset + i) < optionCount; i++) {
    int actualIndex = ui.menu.offset + i;
    uint8_t y = 23 + (i * 13);

    if (ui.menu.index == actualIndex) {
      u8g2.drawBox(2, y-10, 124, 11);
      u8g2.setDrawColor(0);
    }
    drawString(8, y, 8, options[actualIndex]);
    u8g2.setDrawColor(1);
  }
  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 
  if (ui.action.encBtn == BTN_PRESSED) {
    switch(ui.menu.index) {
      case 0: ui.position = UI_SET_TIME; break;
      case 1: ui.position = UI_SET_ALARM; break;
      case 2: ui.position = UI_SET_ELEVATION; break;
      case 3: ui.position = UI_HOME; break;
      case 4: ui.position = UI_SET_SNOOZETIME; break;
      case 5: ui.position = UI_SET_WIFI; break;
      case 6: ui.position = UI_SET_DUMMY; break;
      case 7: ui.position = UI_SET_SECONDSDRIFT; break;
      case 8: ui.position = UI_SET_BACKLIGHT; break;
      case 9: ui.position = UI_SET_REALTIMEDATA; break; 
      default: ui.position = UI_MENU; break;
    }
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      if (ui.menu.index < optionCount -1) {ui.menu.index ++;}
      ui.action.encSteps --;
    } else if (ui.action.encSteps < 0) {
      if (ui.menu.index > 0) {ui.menu.index --;}
      ui.action.encSteps ++;
    }
    ui.refresh = true;
  }
}

void displaySetTime() {
  static uint8_t onEnter = 0;
  static uint8_t selectField = 0;
  char string[30];
  snprintf(string, sizeof(string), "TIME SET");
  drawString(35, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);
  static struct tm calendar;

  if (onEnter == 0) {
    calendar.tm_hour = sysTime.calendar.tm_hour;
    calendar.tm_min = sysTime.calendar.tm_min;
    calendar.tm_sec = sysTime.calendar.tm_sec;
    calendar.tm_mday = sysTime.calendar.tm_mday;
    calendar.tm_mon = sysTime.calendar.tm_mon;
    calendar.tm_year = sysTime.calendar.tm_year;
    onEnter = 1;
  }

  snprintf(string, sizeof(string), "%02d:%02d.%02d", calendar.tm_hour, calendar.tm_min, calendar.tm_sec);
  drawString(4, 38, 26, string);
  snprintf(string, sizeof(string), "%2d/%02d/%04d", calendar.tm_mday, calendar.tm_mon + 1, calendar.tm_year+1900);
  drawString(25, 55, 8, string);

  switch(selectField) {
    case 0: u8g2.drawHLine(6, 40, 31); break;
    case 1: u8g2.drawHLine(48, 40, 31); break;
    case 2: u8g2.drawHLine(92, 40, 31); break;
    case 3: u8g2.drawHLine(25, 57, 14); break;
    case 4: u8g2.drawHLine(50, 57, 14); break;
    case 5: u8g2.drawHLine(70, 57, 34); break;
    default: u8g2.drawHLine(4, 40, 31); break;
  }

  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 
  if (ui.action.encBtn == BTN_PRESSED) {
    selectField ++;
    if (selectField > 5) {
      selectField = 0;
      // back to menu
      setRtcTime(calendar.tm_hour, calendar.tm_min, calendar.tm_sec, calendar.tm_mday, calendar.tm_mon+1, calendar.tm_year+1900);
      onEnter = 0;
      ui.position = UI_MENU;
    }
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      ui.action.encSteps --;
      // increase number
      switch(selectField) {
        case 0: 
          if (calendar.tm_hour == 23)
            calendar.tm_hour = 0;
          else
            calendar.tm_hour ++;
        break;
        case 1: 
          if (calendar.tm_min == 59)
            calendar.tm_min = 0;
          else
            calendar.tm_min ++;
        break;
        case 2: 
          if (calendar.tm_sec == 59)
            calendar.tm_sec = 0;
          else
            calendar.tm_sec ++;
        break;
        case 3: 
          if (calendar.tm_mday == 31)
            calendar.tm_mday = 1;
          else
            calendar.tm_mday ++;
        break;
        case 4: 
          if (calendar.tm_mon == 11)
            calendar.tm_mon = 0;
          else
            calendar.tm_mon ++;
        break;
        case 5: 
          calendar.tm_year ++;
        break;
      }
    } else if (ui.action.encSteps < 0) {
      ui.action.encSteps ++;
      // decrease number
      switch(selectField) {
        case 0: 
          if (calendar.tm_hour == 0)
            calendar.tm_hour = 23;
          else
            calendar.tm_hour --;
        break;
        case 1: 
          if (calendar.tm_min == 0)
            calendar.tm_min = 59;
          else
            calendar.tm_min --;
        break;
        case 2: 
          if (calendar.tm_sec == 0)
            calendar.tm_sec = 59;
          else
            calendar.tm_sec --;
        break;
        case 3: 
          if (calendar.tm_mday == 1)
            calendar.tm_mday = 31;
          else
            calendar.tm_mday --;
        break;
        case 4: 
          if (calendar.tm_mon == 0)
            calendar.tm_mon = 11;
          else
            calendar.tm_mon --;
        break;
        case 5: 
          calendar.tm_year --;
        break;
      }
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.refresh = true;
  }
}

void displaySetAlarm() {
  static bool onEnter = 0;
  static uint8_t selectField = 0;
  char string[30];
  snprintf(string, sizeof(string), "ALARM SET");
  drawString(30, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t dow;  // bitmask as xlmmgvsd
    bool enabled;
  }thisAlarm_s;

  static thisAlarm_s tAlarm;

  if (onEnter == 0) {
    tAlarm.hour = eeprom.data.alarm.hour;
    tAlarm.min =  eeprom.data.alarm.min;
    tAlarm.dow = eeprom.data.alarm.dow;
    onEnter = 1;
  }

  snprintf(string, sizeof(string), "%02d:%02d", tAlarm.hour, tAlarm.min);
  drawString(4, 38, 26, string);
  
  for(int i=0; i<7; i++) {
    uint8_t x = 5 + i * 15;
    snprintf(string, sizeof(string), "%s", wDaysShort[i]);
    drawString(x, 55, 8, string);
    if(tAlarm.dow & (1 << i)) {
      u8g2.drawLine(x, 47, x+6, 53);
      u8g2.drawLine(x+6, 47, x, 53);
    }
  }

  switch(selectField) {
    case 0: u8g2.drawHLine(6, 40, 31); break;   // hour
    case 1: u8g2.drawHLine(48, 40, 31); break;  // minute
    case 2: u8g2.drawHLine(4, 57, 10); break;  // domenica
    case 3: u8g2.drawHLine(19, 57, 10); break;  // lun
    case 4: u8g2.drawHLine(33, 57, 10); break;  // mar
    case 5: u8g2.drawHLine(48, 57, 10); break;  // mer
    case 6: u8g2.drawHLine(64, 57, 10); break;  // gio
    case 7: u8g2.drawHLine(78, 57, 10); break;  // ven
    case 8: u8g2.drawHLine(94, 57, 10); break;  // sab
    default: u8g2.drawHLine(6, 40, 31); break;
  }

  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 

  if (ui.action.encBtn == BTN_PRESSED) {
    selectField ++;
    if (selectField > 8) {
      selectField = 0;
      eeprom.data.alarm.hour = tAlarm.hour;
      eeprom.data.alarm.min = tAlarm.min;
      eeprom.data.alarm.dow = tAlarm.dow;
      // back to menu
      onEnter = 0;
      ui.position = UI_MENU;
    }
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      ui.action.encSteps --;
      // increase number
      switch(selectField) {
        case 0: 
          if (tAlarm.hour == 23)
            tAlarm.hour = 0;
          else
            tAlarm.hour ++;
        break;
        case 1: 
          if (tAlarm.min == 59)
            tAlarm.min = 0;
          else
            tAlarm.min ++;
        break;
        case 2: 
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
          tAlarm.dow ^= (1 << (selectField - 2));
        break;
      }
    } else if (ui.action.encSteps < 0) {
      ui.action.encSteps ++;
      // decrease number
      switch(selectField) {
        case 0: 
          if (tAlarm.hour == 0)
            tAlarm.hour = 23;
          else
            tAlarm.hour --;
        break;
        case 1: 
          if (tAlarm.min == 0)
            tAlarm.min = 59;
          else
            tAlarm.min --;
          break;
        case 2: 
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
          tAlarm.dow ^= (1 << (selectField - 2));
        break;
      }
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.refresh = true;
  }
}

void displaySetElevation() {
  static bool onEnter = false;
  static int16_t currentElevation = 0;
  char string[30];
  snprintf(string, sizeof(string), "ALTITUDE SET");
  drawString(13, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  if (onEnter == false) {
    currentElevation = eeprom.data.info.altitude;
    onEnter = true;
  }

  //snprintf(string, sizeof(string),"%4.0d m", currentElevation);
  snprintf(string, sizeof(string), "%4" PRId16 " m", currentElevation);
  drawString(10, 45, 26, string);


  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 
  if (ui.action.encBtn == BTN_PRESSED) {
    eeprom.data.info.altitude = currentElevation;
    ui.position = UI_MENU;
    myTimers.rst.uiTout = LCD_UI_TOUT_SHORT;
    onEnter = false;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      ui.action.encSteps --;
      // increase number
      currentElevation ++;

    } else if (ui.action.encSteps < 0) {
      ui.action.encSteps ++;
      // decrease number
      currentElevation --;
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.refresh = true;
  }
}

void displaySetSnoozetime(void)
{
  static bool onEnter = false;
  static uint8_t currVal = 0;
  char string[30];
  snprintf(string, sizeof(string), "SNOOZE SET");
  drawString(13, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  if (onEnter == false) {
    currVal = eeprom.data.alarm.snoozeMin;
    onEnter = true;
  }

  snprintf(string, sizeof(string),"%2u min", currVal);
  drawString(10, 45, 26, string);


  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 
  if (ui.action.encBtn == BTN_PRESSED) {
    eeprom.data.alarm.snoozeMin = currVal;
    ui.position = UI_MENU;
    myTimers.rst.uiTout = LCD_UI_TOUT_SHORT;
    onEnter = false;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      ui.action.encSteps --;
      // increase number
      if (currVal < 15)
        currVal ++;
    } else if (ui.action.encSteps < 0) {
      ui.action.encSteps ++;
      // decrease number
      if (currVal >=2)
        currVal --;
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.refresh = true;
  }
}

void displaySetWiFi(void)
{
  static bool onEnter = false;
  static uint8_t selectField = 0;
  static uint8_t bklTout = 0;
  static uint8_t bklPerc = 0;
  static uint8_t stbTout = 0;
  char string[30];
  snprintf(string, sizeof(string), "WIFI Set");
  drawString(1, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  backlightOn();
  myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
  
  if (ui.action.encBtn == BTN_PRESSED) {
    ui.position = UI_MENU;
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    backlightOff();
  }
}

void displaySetDummy(void)
{
  static bool onEnter = false;
  static uint8_t selectField = 0;
  static uint8_t bklTout = 0;
  static uint8_t bklPerc = 0;
  static uint8_t stbTout = 0;
  char string[30];
  snprintf(string, sizeof(string), "DUMMY SCREEN");
  drawString(1, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  backlightOn();
  myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
  
  if (ui.action.encBtn == BTN_PRESSED) {
    ui.position = UI_MENU;
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    backlightOff();
  }
}

void displaySetSecondsdrift(void) 
{
  static bool onEnter = false;
  static int16_t currVal = 0;
  char string[30];
  snprintf(string, sizeof(string), "DRIFT SET SEC");
  drawString(1, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  if (onEnter == false) {
    currVal = eeprom.data.time.secondsDriftPerDay;
    onEnter = true;
  }

  
  snprintf(string, sizeof(string), "%4" PRId16 " s", currVal);
  drawString(10, 40, 26, string);

  snprintf(string, sizeof(string), "Used to correct");
  drawString(0, 50, 8, string);
  snprintf(string, sizeof(string), "RTC accuracy");
  drawString(0, 60, 8, string);

  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 
  if (ui.action.encBtn == BTN_PRESSED) {
    eeprom.data.time.secondsDriftPerDay = currVal;
    ui.position = UI_MENU;
    myTimers.rst.uiTout = LCD_UI_TOUT_SHORT;
    onEnter = false;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      ui.action.encSteps --;
      // increase number
      if (currVal < 999)
        currVal ++;
    } else if (ui.action.encSteps < 0) {
      ui.action.encSteps ++;
      // decrease number
      if (currVal >-999)
        currVal --;
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.refresh = true;
  }
}

void displaySetBacklight(void) 
{
  static bool onEnter = false;
  static uint8_t selectField = 0;
  static uint8_t bklTout = 0;
  static uint8_t bklPerc = 0;
  static uint8_t stbTout = 0;
  char string[30];
  snprintf(string, sizeof(string), "BACKLIGHT SET");
  drawString(1, 9, 8, string);
  u8g2.drawHLine(1, 10, 128);

  if (onEnter == false) {
    bklTout = eeprom.data.backlight.durationSec;
    bklPerc = eeprom.data.backlight.perc;
    stbTout = eeprom.data.backlight.stbToutSec;
    onEnter = true;
  }


  snprintf(string, sizeof(string),"Tout:");
  drawString(2, 25, 8, string);
  snprintf(string, sizeof(string),"%2d s", bklTout);
  drawString(80, 25, 8, string);
  snprintf(string, sizeof(string),"Perc:");
  drawString(2, 40, 8, string);
  snprintf(string, sizeof(string),"%3d% %", bklPerc);
  drawString(80, 40, 8, string);
  snprintf(string, sizeof(string),"Stb Tout:");
  drawString(2, 55, 8, string);
  snprintf(string, sizeof(string),"%2d s", stbTout);
  drawString(80, 55, 8, string);
  
  switch(selectField) {
    case 0: u8g2.drawHLine(80, 28, 30); break;
    case 1: u8g2.drawHLine(80, 43, 30); break;
    case 2: u8g2.drawHLine(80, 58, 30); break;
    default: selectField = 0; break;
  }

  ui.refresh = false;

  if (ui.action.topBtn == BTN_PRESSED) {
    backlightOn();
    ui.action.topBtn = BTN_RELEASED;
    ui.refresh = true;
  } 
  if (ui.action.encBtn == BTN_PRESSED) {
    selectField ++;
    if(selectField > 2) {
      selectField = 0;
      eeprom.data.backlight.durationSec = bklTout;
      eeprom.data.backlight.perc = bklPerc;
      eeprom.data.backlight.stbToutSec = stbTout;
      onEnter = false;
      ui.position = UI_MENU;
      backlightOff();
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
  } 

  if (ui.action.encSteps != 0) {
    if (ui.action.encSteps > 0) {
      ui.action.encSteps --;
      // increase number
      switch(selectField) {
        case 0: if (bklTout < 60) {bklTout ++;} break;
        case 1: 
          if (bklPerc < 100) {
              bklPerc ++;
              setBacklight(1, bklPerc, 1);
            } 
          break;
        case 2: if (stbTout < 120) {stbTout ++;} break;
        default: selectField = 0; break;        
      }
    } else if (ui.action.encSteps < 0) {
      ui.action.encSteps ++;
      // decrease number
      switch(selectField) {
        case 0: if (bklTout > 5) {bklTout --;} break;
        case 1: 
          if (bklPerc > 0) {
            bklPerc --;
            setBacklight(1, bklPerc, 1);
          }
          break;
        case 2: if (stbTout > 30) {stbTout --;} break;
        default: selectField = 0; break;        
      }
    }
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.refresh = true;
  }
}

void displayRealtimeData() 
{
  static bool onEnter = false;
  static uint8_t selectField = 0;
  static uint8_t bklTout = 0;
  static uint8_t bklPerc = 0;
  static uint8_t stbTout = 0;
  char string[50];
  snprintf(string, sizeof(string), "DEBUG Vb:%1.2f %u", ambData.vBatt, millis()/1000);
  drawString(0, 7, 6, string);
  u8g2.drawHLine(0, 8, 128);

  snprintf(string, sizeof(string),"Time:%ums", (int(ambData.bsec2.timestamp / INT64_C(1000000))));
  drawString(0, 17, 6, string);
  snprintf(string, sizeof(string),"%.1fC %.0f%% %.0fhPa", ambData.bsec2.temperature, ambData.bsec2.humidity, ambData.bsec2.pressure);
  drawString(0, 25, 6, string);
  snprintf(string, sizeof(string),"Stab:%d RunIn:%d Accu:%d", ambData.bsec2.status.stabilize, ambData.bsec2.status.runin, ambData.bsec2.status.accuracy);
  drawString(0, 33, 6, string);
  snprintf(string, sizeof(string),"eCO2:%.0fppm IAQ:%.0f", ambData.bsec2.debug.eCO2, ambData.bsec2.debug.IAQ);
  drawString(0, 41, 6, string);
  snprintf(string, sizeof(string),"bVOC:%.1fppm gas:%.1f%%", ambData.bsec2.debug.bVOC, ambData.bsec2.debug.gasPerc);
  drawString(0, 49, 6, string);
  
  backlightOn();
  myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
  
  if (ui.action.encBtn == BTN_PRESSED) {
    ui.position = UI_MENU;
    myTimers.rst.uiTout = LCD_UI_TOUT_LONG;
    ui.action.encBtn = BTN_RELEASED;
    ui.refresh = true;
    backlightOff();
  }
}