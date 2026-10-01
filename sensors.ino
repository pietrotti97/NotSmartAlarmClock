#include "bme68xLibrary.h"
#include <bme68x/bme68x.h> 
//#include <bsec2.h>

#define BME68X_I2C_ADDR 0x76
/*
Bsec2 bsec;
const uint8_t bsec_config[] = {
  #include "bme688/bme688_reg_18v_300s_4d/bsec_selectivity.txt"
};*/

typedef enum {
  PHASE_INIT,
  PHASE_MEAS_START,
  PHASE_MEAS_WAIT,
  PHASE_MEAS_READ,
  PHASE_SET_NEXT,
  PHASE_NOF
} sensorPhase_e;

typedef struct {
  bool initOk;
  sensorPhase_e phase;
  uint8_t cycleCounter;
} sensorStatus_s;
sensorStatus_s sensData;

Bme68x bme;

#define READ_GAS_EVERY_N  5

void initSensor(void)  { 
  if (sensData.initOk == true) { return; }
  bme.begin(BME68X_I2C_ADDR, Wire);

  if(bme.checkStatus()!= BME68X_OK) {
    logPrintf("\nError initializing BME688 sensor");
    sensData.initOk = false;
    return;
  }
  bme.setTPH(BME68X_OS_16X, BME68X_OS_2X, BME68X_OS_8X);
  bme.setOpMode(BME68X_SLEEP_MODE);

  logPrintf("\n\rBME688 Init Ok");

  sensData.initOk = true;
}

void readSensor(void) 
{
  switch(sensData.phase) {
    case PHASE_INIT: {
      initSensor();
      sensData.phase = PHASE_SET_NEXT;
    } break;
    
    case PHASE_MEAS_START: {
      if (!sensData.initOk) {
        sensData.phase = PHASE_INIT;
        break;
      }
      uint32_t measTimeMs;
      bme.setOpMode(BME68X_SLEEP_MODE);
      bme.setTPH(BME68X_OS_16X, BME68X_OS_2X, BME68X_OS_8X);
      if (sensData.cycleCounter == READ_GAS_EVERY_N) {
        bme.setHeaterProf(320, 150);
        measTimeMs = (bme.getMeasDur(BME68X_FORCED_MODE) / 1000) + 150 + 1;
      } else {  
        bme.setHeaterProf(0, 0);
        measTimeMs = (bme.getMeasDur(BME68X_FORCED_MODE) / 1000)  + 1;
     }
      bme.setOpMode(BME68X_FORCED_MODE);
      //uint32_t meas_dur = bme.getMeasDur(BME68X_FORCED_MODE); // returns microseconds
      myTimers.rst.BME68xMeasTimerElapsed = measTimeMs;
      sensData.phase = PHASE_MEAS_WAIT;
    } break;

    case PHASE_MEAS_WAIT: {
      if (!sensData.initOk) {
        sensData.phase = PHASE_INIT;
        break;
      }
      if (myTimers.rst.BME68xMeasTimerElapsed == 0) { sensData.phase = PHASE_MEAS_READ; }
    } break;

    case PHASE_MEAS_READ: {
      if (!sensData.initOk) {
        sensData.phase = PHASE_INIT;
        break;
      }
      bme68xData data;
      if (bme.fetchData()) {
        bme.getData(data);
    
        ambData.temperature = data.temperature;
        ambData.humidity = data.humidity;
        ambData.pressure = data.pressure/100;
        pressureAddVal(data.pressure, data.temperature);

        logPrintf("\n\rTemp: %2.1f °C | Press: %4.1f hPa | Umidità: %2.0f% %", data.temperature, data.pressure/100, data.humidity);

        if (sensData.cycleCounter == READ_GAS_EVERY_N) {
          logPrintf(" NEW=%d, GAS_VALID=%d, HEAT_STAB=%d", 
                      (data.status & BME68X_NEW_DATA_MSK) != 0, 
                              (data.status & BME68X_GASM_VALID_MSK) != 0, 
                                            (data.status & BME68X_HEAT_STAB_MSK) != 0);
          uint8_t validGas = BME68X_GASM_VALID_MSK | BME68X_HEAT_STAB_MSK | BME68X_NEW_DATA_MSK;
          if ((data.status & validGas) == validGas) {

          logPrintf(" | Gas: %4.0f kOhm", data.gas_resistance/1000.0);
          int64_t timestamp_ns = (int64_t)millis() * 1000000; 
          //measureVocBSECLib(data.temperature, data.humidity, data.pressure, data.gas_resistance, timestamp_ns);
          } else {
            logPrintf(" | Gas: Non valido");
          }
        }
      } else {
        logPrintf("BME68x Read Error!");
      }
      sensData.phase = PHASE_SET_NEXT; 
    } break;

    case PHASE_SET_NEXT: {
      if (!sensData.initOk) {
        sensData.phase = PHASE_INIT;
        break;
      }
      if (myTimers.rst.sensor == 0) { 
        sensData.cycleCounter ++;
        if (sensData.cycleCounter > READ_GAS_EVERY_N) { 
          sensData.cycleCounter = 0;
        }
        sensData.phase = PHASE_MEAS_START; 
        myTimers.rst.sensor = 10;
        }
    } break;

    default: sensData.phase = PHASE_INIT; break;     
  }
}
