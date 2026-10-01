#define PRESS_MINUTES_STORE 720 //12 hours
#define PRESS_FETCH_TIME_S  60  // fetch the value every 60 seconds

typedef struct {
  float buf[PRESS_MINUTES_STORE];
  uint16_t count;
  bool full;
}dataPress_s;

dataPress_s dataP;

void pressureAddVal(float pressVal, float temperature)
{
  static uint32_t oldTime = 0;
  if (millis() - oldTime >= 60000 || oldTime == 0) {
    float altitude = eeprom.data.info.altitude;
    float tempStandardK = 288.15 - (0.0065 * altitude);

    float corrPressure = pressVal / pow(1.0 - (0.0065 * altitude) / tempStandardK, 5.255);
    dataP.buf[dataP.count] = corrPressure;
    dataP.count ++;
    if (dataP.count >= PRESS_MINUTES_STORE) {
      dataP.count = 0;
      dataP.full = true;
    }
    oldTime = millis();
  }
}

void calcWeather(void)
{
  static uint16_t oldIndex = 0;
  bool forecastReady = false;
  if (oldIndex == dataP.count) { return; } // no new data present

  uint16_t dataAvail = dataP.full ? PRESS_MINUTES_STORE : dataP.count;
  if (dataAvail < 180) {
    oldIndex = dataP.count; // not enough data
    return;
  }

  // regressione lineare, somma sulle 3h
  float sumX_3h = 0, sumY_3h = 0, sumXY_3h = 0, sumX2_3h = 0;
  uint16_t n_3h = 180;  // analyze 180 minutes of data
  for(uint16_t i=0; i<n_3h; i++) {
    int16_t idx = (int16_t)dataP.count - 1 - i;
    if (idx < 0) idx += PRESS_MINUTES_STORE;

    float x = (float)i;
    float y = dataP.buf[idx];

    sumX_3h += x;
    sumY_3h += y;
    sumXY_3h += x * y;
    sumX2_3h += x * x;
  }
  float slope3h = (n_3h * sumXY_3h - sumX_3h * sumY_3h) / (n_3h * sumX2_3h - sumX_3h * sumX_3h);  // pendenza sulle 3h
  float delta3h_filtered = -slope3h * 180.0;  // inversione del tempo

  float sumX_12h = 0, sumY_12h = 0, sumXY_12h = 0, sumX2_12h = 0;
  uint16_t n_12h = dataAvail; // itero sui dati disponibili

  for (uint16_t i = 0; i < n_12h; i++) {
    int16_t idx = (int16_t)dataP.count - 1 - i;
    if (idx < 0) idx += PRESS_MINUTES_STORE;
    float x = (float)i;
    float y = dataP.buf[idx];
    sumX_12h += x;
    sumY_12h += y;
    sumXY_12h += x * y;
    sumX2_12h += x * x;
  }
  float slope12h = (n_12h * sumXY_12h - sumX_12h * sumY_12h) / (n_12h * sumX2_12h - sumX_12h * sumX_12h);
  float delta12h_filtered = -slope12h * (float)n_12h;


  calcWeatherForecast(delta3h_filtered, delta12h_filtered);
  oldIndex = dataP.count;
}

void calcWeatherForecast(float delta3h, float delta12h) 
{
  float altCorr = 1.0 - (eeprom.data.info.altitude * 0.00011);
  if (altCorr < 0.7) altCorr = 0.7;

  float thresh_fastDown = -3.0 * altCorr;
  float thresh_midDown = -2.0 * altCorr;
  float thresh_slowDown = -1.5 * altCorr;
  float thresh_fastUp = 3.0 * altCorr;
  float thresh_midUp = 2.0 * altCorr;
  float thresh_slowUp = 1.5 * altCorr;
  
  if (delta3h <= thresh_fastDown) {
    ambData.forecastVal = FORECAST_TEMPORALE_VENTO;         // Crollo rapido, forte maltempo e vento
  } else if (delta3h <= thresh_slowDown && delta3h > thresh_fastDown) {
    // 2. SCENARIO PEGGIORAMENTO (Pressione in calo)
    if (delta12h < thresh_midDown) {
      ambData.forecastVal = FORECAST_PIOGGIA_CONTINUA;      // Peggioramento esteso, pioggia diffusa
    } else {
      if (ambData.humidity > 75.0) {
        ambData.forecastVal = FORECAST_PIOGGIA_IMMINENTE;   // Calo rapido, pioggia a breve termine
      } else {
        ambData.forecastVal = FORECAST_INSTABILE;           // Poco nuvoloso / Instabilità passeggera
      }
    }
  } else if (delta3h >= thresh_slowUp) {
    // 3. SCENARIO MIGLIORAMENTO (Pressione in aumento)
    if (delta12h > thresh_midUp) {
      if (ambData.humidity < 40.0) {
        ambData.forecastVal = FORECAST_SOLE_SECCO;      // Soleggiato, asciutto, alta pressione
      } else {
        ambData.forecastVal = FORECAST_STABILE_SERENO;  // Bel tempo, stabile e senza variazioni
      }
    } else {
      ambData.forecastVal = FORECAST_VARIBILE_MIGLIORAMENTO;  // In miglioramento con schiarite
    }
  } else {
    // 4. SCENARIO STABILE (La pressione oscilla poco nelle 3 ore, es. tra -1.5 e +1.5 hPa)
    if (delta12h <= thresh_midDown) {
      ambData.forecastVal = FORECAST_LENTO_PEGGIORAMENTO;   // Tendenza al lento peggioramento, nuvole
    } else if (delta12h >= thresh_midUp) {
      if (ambData.humidity < 40.0) {
        ambData.forecastVal = FORECAST_SOLE_SECCO;
      } else {
        ambData.forecastVal = FORECAST_STABILE_SERENO;
      }
    } else {
      if (ambData.humidity > 80.0) {
        if (eeprom.data.info.altitude < 500) {
          ambData.forecastVal = FORECAST_NEBBIA_FOSCHIA;
        } else {
          ambData.forecastVal = FORECAST_NUVOLOSO_STABILE;
        }
      } else {
          ambData.forecastVal = FORECAST_NUVOLOSO_STABILE;
      }
    }
  }

  logPrintf("\n\r[PREVISIONE] Delta 3h: %.2f hPa | Delta 12h: %.2f hPa -> Verdetto: %d", delta3h, delta12h, ambData.forecastVal);
}

