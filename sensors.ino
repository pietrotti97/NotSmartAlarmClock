#include "bme68xLibrary.h"
#include <bme68x/bme68x.h> 
#include <bsec2.h>

#define BME68X_I2C_ADDR 0x76

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

#define USE_LIB_BSEC2
Bsec2 envSensor;
const uint8_t bsec_config[] = {
  #include "bme688/bme688_reg_18v_300s_4d/bsec_selectivity.txt"
  //#include "bme688/bsec_iaq_generic_33v_3s_4d.txt"
};

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
    
        ambData.bsec2.temperature = data.temperature;
        ambData.bsec2.humidity = data.humidity;
        ambData.bsec2.pressure = data.pressure/100;
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


void initSensorBSEC2(void)
{ 
  if (sensData.initOk == true) { return; }

  bsecSensor sensorList[] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    //BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    //BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    //BSEC_OUTPUT_RAW_GAS_INDEX,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_GAS_PERCENTAGE,
    BSEC_OUTPUT_COMPENSATED_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS
    //BSEC_OUTPUT_REGRESSION_ESTIMATE_1,
    //BSEC_OUTPUT_REGRESSION_ESTIMATE_2,
    //BSEC_OUTPUT_REGRESSION_ESTIMATE_3,
    //BSEC_OUTPUT_REGRESSION_ESTIMATE_4
  };

  if (!envSensor.begin(BME68X_I2C_ADDR, Wire)) {
    logPrintf("\n\rError initializing BME688 sensor!");
    sensData.initOk = false;
    return;
  }

	//if (!envSensor.setConfig(bsec_config)) { 
  //  logPrintf("\n\rError Bsec Config, ");
  //  checkBsecStatus (envSensor); 
  //  return;
  //}

  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_ULP)) {  // BSEC_SAMPLE_RATE_SCAN //BSEC_SAMPLE_RATE_LP //BSEC_SAMPLE_RATE_ULP
    logPrintf("\n\rError Bsec Subscription, ");
    checkBsecStatus (envSensor);
    return;
  }
  envSensor.attachCallback(dataCallbackBSEC2);

  logPrintf("\n\rBME688 BSEC2 Init Ok");
  logPrintf("\nBSEC library version %d.%d.%d.%d", envSensor.version.major, envSensor.version.minor, envSensor.version.major_bugfix, envSensor.version.minor_bugfix);
  sensData.initOk = true;
}

void readSensorBSEC2(void)
{
  if (sensData.initOk == false) {
    initSensorBSEC2();
  } else {
    if (!envSensor.run()) {
      checkBsecStatus(envSensor);
    }
  }
}

void dataCallbackBSEC2(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
  if (!outputs.nOutputs)
      return;
  uint8_t index = 0;
  
  logPrintf("\n\rBSEC: Timestamp = %u, ",(int(outputs.output[0].time_stamp / INT64_C(1000000))));
  uint64_t newTimestamp = outputs.output[0].time_stamp;
  if (newTimestamp == ambData.bsec2.timestamp) {
    return;
  }
  ambData.bsec2.timestamp = newTimestamp;
  uint8_t stabilize = 0;
  uint8_t accuracy = 0;
  uint8_t runin = 0;

  for (uint8_t i = 0; i < outputs.nOutputs; i++) {
      const bsecData output  = outputs.output[i];
      switch (output.sensor_id){
        case BSEC_OUTPUT_RAW_TEMPERATURE:
          logPrintf("Temp: %2.2f, ", output.signal);
          ambData.bsec2.temperature = output.signal;
          break;
        case BSEC_OUTPUT_RAW_PRESSURE:
          //Serial.println("\tPressure = " + String(output.signal));
          logPrintf("Press: %4.2f, ", output.signal);
          ambData.bsec2.pressure = output.signal;
          break;
        case BSEC_OUTPUT_RAW_HUMIDITY:
          //Serial.println("\tHumidity = " + String(output.signal));
          logPrintf("RH: %2.2f%%, ", output.signal);
          ambData.bsec2.humidity = output.signal;
          break;
        case BSEC_OUTPUT_RAW_GAS:
          //Serial.println("\tGas resistance = " + String(output.signal));
          logPrintf("Gas Ohm: %.2f kOhm, ", output.signal/1000);
          break;
        case BSEC_OUTPUT_RAW_GAS_INDEX:
          //Serial.println("\tGas index = " + String(output.signal));
          logPrintf("Gas Index: %2.2f, ", output.signal);
          break;
        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
          //Serial.println("\tCompensated temperature = " + String(output.signal)); 
          logPrintf("Comp Temp: %2.2f, ", output.signal);
          break;
        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
          //Serial.println("\tCompensated humidity = " + String(output.signal));
          logPrintf("Comp RH: %2.2f %, ", output.signal);
          break;
        case BSEC_OUTPUT_IAQ:
          logPrintf("IAQ: %.2f (Acc: %d), ", output.signal, output.accuracy);
          ambData.bsec2.debug.IAQ = output.signal;
          accuracy = output.accuracy;
        break;
        case BSEC_OUTPUT_CO2_EQUIVALENT:
          logPrintf("eCO2: %.2f ppm, ", output.signal);
          ambData.bsec2.debug.eCO2 = output.signal;
        break;
        case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
          logPrintf("bVOC: %.3f ppm, ", output.signal);
          ambData.bsec2.debug.bVOC = output.signal;
        break;
        case BSEC_OUTPUT_GAS_PERCENTAGE:
          logPrintf("Gas: %.2f %%, ", output.signal);
          ambData.bsec2.debug.gasPerc = output.signal;
        break;
        case BSEC_OUTPUT_COMPENSATED_GAS:
          logPrintf("Gas Comp: %.3f, ", output.signal);
        break;
        case BSEC_OUTPUT_STABILIZATION_STATUS:
          logPrintf("Stabilize: %.0f, ", output.signal);
          stabilize = output.signal;
        break;
        case BSEC_OUTPUT_RUN_IN_STATUS:
          logPrintf("RunIn: %.0f, ", output.signal);
          runin = output.signal;
        break;

        case BSEC_OUTPUT_GAS_ESTIMATE_1:
        case BSEC_OUTPUT_GAS_ESTIMATE_2:
        case BSEC_OUTPUT_GAS_ESTIMATE_3:
        case BSEC_OUTPUT_GAS_ESTIMATE_4:
            index = (output.sensor_id - BSEC_OUTPUT_GAS_ESTIMATE_1);
            if (index == 0) // The four classes are updated from BSEC with same accuracy, thus printing is done just once.
            {
              //Serial.println("\tAccuracy = " + String(output.accuracy));
              logPrintf("Accuracy: %2.2f, ", output.accuracy);
            }
            //Serial.println("\tClass " + String(index + 1) + " probability = " + String(output.signal * 100) + "%");
            logPrintf("Class: %d, Probability: %f %, ", index + 1, (output.signal * 100));
            break;
        case BSEC_OUTPUT_REGRESSION_ESTIMATE_1:
        case BSEC_OUTPUT_REGRESSION_ESTIMATE_2:
        case BSEC_OUTPUT_REGRESSION_ESTIMATE_3:
        case BSEC_OUTPUT_REGRESSION_ESTIMATE_4:
            index = (output.sensor_id - BSEC_OUTPUT_REGRESSION_ESTIMATE_1);
            if (index == 0) // The four targets are updated from BSEC with same accuracy, thus printing is done just once.
            {
              //Serial.println("\tAccuracy = " + String(output.accuracy));
              logPrintf("Accuracy: %2.2f, ", output.accuracy);
            }
            //Serial.println("\tTarget " + String(index + 1) + " = " + String(output.signal * 100));
            logPrintf("Target: %d = %f , ", index + 1, (output.signal * 100));
            break;
        default:
            break;
      }
  }
  if ((stabilize) && (runin) && (accuracy >= 2)) {
    ambData.bsec2.eCO2 = ambData.bsec2.debug.eCO2;
    ambData.bsec2.bVOC = ambData.bsec2.debug.bVOC;
    ambData.bsec2.IAQ = ambData.bsec2.debug.IAQ;
    ambData.bsec2.gasPerc = ambData.bsec2.debug.gasPerc;
  } else {
    ambData.bsec2.eCO2 = -999.0f;
    ambData.bsec2.bVOC = -999.0f;
    ambData.bsec2.IAQ = -999.0f;
    ambData.bsec2.gasPerc = -999.0f;
  }
  ambData.bsec2.status.stabilize = stabilize;
  ambData.bsec2.status.runin = runin;
  ambData.bsec2.status.accuracy = accuracy;
  pressureAddVal(ambData.bsec2.pressure, ambData.bsec2.temperature);
}


void checkBsecStatus(Bsec2 bsec)
{
  if (bsec.status < BSEC_OK) {
    logPrintf("BSEC error code : %d", bsec.status);
    /* Halt in case of failure */
  } else if (bsec.status > BSEC_OK) {
    logPrintf("BSEC warning code : %d", bsec.status);
  }

  if (bsec.sensor.status < BME68X_OK) {
    logPrintf("\n\rBME68X error code : %d", bsec.sensor.status);
    /* Halt in case of failure */
  } else if (bsec.sensor.status > BME68X_OK) {
    logPrintf("\n\rBME68X warning code : %d", bsec.sensor.status);
  }
}


void initSensorFunc(void)
{
#ifdef USE_LIB_BSEC2
  initSensorBSEC2();
#else
  initSensor();
#endif
}

void readSensorFunc(void)
{
#ifdef USE_LIB_BSEC2
  readSensorBSEC2();
#else
  readSensor();
#endif
}