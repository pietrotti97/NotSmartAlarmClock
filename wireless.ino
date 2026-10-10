#include <WiFi.h>
#include "esp_sntp.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>



#define TIMEOUT_MS 30000
#define RETRY_DELAY_MS 120000

typedef enum {
  WL_STATUS_OFF,
  WL_STATUS_BEGIN,
  WL_STATUS_CONNECTING,
  WL_STATUS_CONNECTED,
  WL_STATUS_GETIP,
  WL_STATUS_NTPSYNC,
  WL_STATUS_NTPSYNCWAIT,
  WL_STATUS_NTPSUCCESS,
  WL_STATUS_CHECKFWUPDATE,
  WL_STATUS_EXECFWUPDATE,
  WL_STATUS_ERROR,
  WL_STATUS_DISCONNECT
}wifiStatus_e;

typedef struct {
  wifiStatus_e status;
  struct {
    time_t oldTime;
    uint64_t olduS;
  }timeCorrection;
  uint64_t currSysTime;
}wifiData_s;
wifiData_s wifiData;

void wifiHandler(void)
{
  switch(wifiData.status) {
    case WL_STATUS_OFF:
      break;
    case WL_STATUS_BEGIN: {
      memset(&wifiData, 0, sizeof(wifiData_s));
      memset(&rtcBkp.wifi.lastConn, 0, sizeof(rtcBkp.wifi.lastConn));
      WiFi.disconnect(true); 
      WiFi.mode(WIFI_OFF);
      delay(10);
      WiFi.mode(WIFI_STA);
      if (strlen(eeprom.data.wifiNet.ssid) < 2 || strlen(eeprom.data.wifiNet.pwd) < 2) {
        wifiData.status = WL_STATUS_OFF;
      } 
      WiFi.begin(eeprom.data.wifiNet.ssid, eeprom.data.wifiNet.pwd);
      logPrintf("\n\rWiFi Credentials: %s, %s", eeprom.data.wifiNet.ssid, eeprom.data.wifiNet.pwd);
      logPrintf("\n\rWiFi Connecting");
      wifiData.currSysTime = millis();
      wifiData.status = WL_STATUS_CONNECTING;
    } break;
    case WL_STATUS_CONNECTING: {
      if (WiFi.status() == WL_CONNECTED) {
        logPrintf("\tConnected !");
        wifiData.status = WL_STATUS_CONNECTED;  
      } else {
        logPrintf(".");
        if ((millis() - wifiData.currSysTime) > TIMEOUT_MS) {
          wifiData.status = WL_STATUS_ERROR;
        }
      }
    } break;
    case WL_STATUS_CONNECTED: {
      int16_t rssi = (int16_t)WiFi.RSSI();
      
      if (rssi >= -50)
        rtcBkp.wifi.lastConn.strength = 4;
      else if (rssi >= -67)
        rtcBkp.wifi.lastConn.strength = 3;
      else if (rssi >= -70)
        rtcBkp.wifi.lastConn.strength = 2;
      else if (rssi >= -80)
        rtcBkp.wifi.lastConn.strength = 2;
      else
        rtcBkp.wifi.lastConn.strength = 1;

      wifiData.status = WL_STATUS_GETIP;
      // maybe here read signal strength
    } break;
    case WL_STATUS_GETIP: {
      IPAddress ip = WiFi.localIP();
      if (ip[0] != 0 && ip[1] != 0 && ip[2] != 0 && ip[3] != 0) {
        rtcBkp.wifi.lastConn.ipAddr[0] = ip[0];
        rtcBkp.wifi.lastConn.ipAddr[1] = ip[1];
        rtcBkp.wifi.lastConn.ipAddr[2] = ip[2];
        rtcBkp.wifi.lastConn.ipAddr[3] = ip[3];
        wifiData.status = WL_STATUS_NTPSYNC;
        logPrintf("> %d.%d.%d.%d", ip[0],ip[1],ip[2],ip[3]);
      } else {
        logPrintf("\n\rWaiting IP Addr:");
      }
    } break;
    case WL_STATUS_NTPSYNC: {
      wifiData.timeCorrection.oldTime = time(NULL);
      wifiData.timeCorrection.olduS = micros();
      configTime((int32_t)(eeprom.data.time.timezone * 3600), eeprom.data.time.isDST*3600, "pool.ntp.org", "time.nist.gov");
      //setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
      //tzset();
      esp_sntp_set_time_sync_notification_cb(callbackNTPSync);
      wifiData.currSysTime = millis();
      wifiData.status = WL_STATUS_NTPSYNCWAIT;
    } break;
    case WL_STATUS_NTPSYNCWAIT: {
      if ((millis() - wifiData.currSysTime) > TIMEOUT_MS) {
        wifiData.status = WL_STATUS_ERROR;
      }
    } break;
    case WL_STATUS_NTPSUCCESS: {
      logPrintf("\n\rNTP Sync SUCCESS");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%A, %B %d %Y %H:%M:%S", &timeinfo);
        logPrintf("\tRTC Localtime: %s", buffer);
      }
      rtcBkp.wifi.lastConn.lastSyncTime = timeinfo;
      uint64_t elapsedTimeSeconds = (micros() - wifiData.timeCorrection.olduS + 500000) / 1000000;
      int64_t oldTimeCorrected = (int64_t)wifiData.timeCorrection.oldTime + elapsedTimeSeconds;
      time_t newTime = time(NULL);
      int16_t timeDrift = (int16_t)((int64_t)newTime - oldTimeCorrected);
      logPrintf("\tRTC Drift is: %d seconds", timeDrift);
      eeprom.data.time.secondsDriftPerDay = timeDrift;
      rtcBkp.wifi.lastConn.result = 2;
      wifiData.status = WL_STATUS_DISCONNECT;
    } break;

    case WL_STATUS_CHECKFWUPDATE: {
      if (checkFwUpdateAvail())
        wifiData.status = WL_STATUS_EXECFWUPDATE;
      else 
        wifiData.status = WL_STATUS_DISCONNECT;
    } break;
    case WL_STATUS_EXECFWUPDATE: {
      if(!FwUpdateDownload()) {
        wifiData.status = WL_STATUS_DISCONNECT;
      }
    } break;
    case WL_STATUS_ERROR: {
      logPrintf("\n\rWiFi ERROR");
      rtcBkp.wifi.lastConn.result = 1;
      wifiData.status = WL_STATUS_OFF;
    } break;
    case WL_STATUS_DISCONNECT: {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      logPrintf("\n\rWiFi Powered OFF");
      wifiData.status = WL_STATUS_OFF;
    } break;
    default:
    break;
  } 
}

void callbackNTPSync(struct timeval *tv) 
{
  wifiData.status = WL_STATUS_NTPSUCCESS;
}

void startWiFi(void)
{
  if (wifiData.status == WL_STATUS_OFF)
    wifiData.status = WL_STATUS_BEGIN;
}

void checkSyncTimeNetwork(void)
{
  static int64_t lastSyncTime = 0;
  if (sysTime.calendar.tm_hour == eeprom.data.wifiNet.syncHour) {
    if(rtcBkp.wifi.lastConn.syncToday == 0) {
      rtcBkp.wifi.lastConn.result = 0;
      rtcBkp.wifi.lastConn.syncToday = 1;
      lastSyncTime = time(NULL);
      startWiFi();
    } else {
      if (rtcBkp.wifi.lastConn.result != 2) {
        if ((time(NULL) - lastSyncTime) > RETRY_DELAY_MS) {
          rtcBkp.wifi.lastConn.syncToday = 0;
        }
      }
    }
  } else {
    rtcBkp.wifi.lastConn.syncToday = 0;
  }
}

uint8_t checkFwUpdateAvail(void)
{
  WiFiClientSecure client_api;
  client_api.setInsecure(); // no rootCA cert for HTTPS
  
  HTTPClient http;
  
  String url_api = "https://github.com" + String(github_user) + "/" + String(github_repo) + "/releases/latest";
  
  logPrintf("Checking for updates...");
  http.begin(client_api, url_api);
  
  http.addHeader("User-Agent", "ESP32-C3-OTA-Client");  // set userAgent

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      int index = payload.indexOf("\"tag_name\":\"");
      if (index != -1) {
          int start = index + 12; 
          int end = payload.indexOf("\"", start);
          String tag_github = payload.substring(start, end);
          
          logPrintf("Tag trovato su GitHub: %s\n", tag_github.c_str());
          
          int github_ver = 0;
          int github_rel = 0;
          sscanf(tag_github.c_str(), "%d.%d", &github_ver, &github_rel);

          if (github_ver > currentFwVer || (github_ver == currentFwVer && github_rel > currentFwRel)) {
            logPrintf("Nuova versione disponibile (%d.%d)! Disconnessione API e avvio OTA...\n", github_ver, github_rel);
            http.end();
            return 1;
          } else {
            logPrintf("Il firmware attuale (%d.%d) è già aggiornato.\n", currentFwVer, currentFwRel);
          }
      } else {
        logPrintf("Errore: Campo 'tag_name' non trovato nel JSON. Controlla di aver creato una Release su GitHub.");
      }
  } else {
    logPrintf("Errore richiesta HTTP API. Codice HTTP: %d (Se -1, verifica la connessione a internet)\n", httpCode);
  }
  http.end();
  return 0; // firmware not available
}

uint8_t FwUpdateDownload(void)
{
  WiFiClientSecure client;
  client.setInsecure(); // no rootCA cert for HTTPS
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // enable redirect tracking
  String fwUrl = "https://github.com" + String(github_user) + "/" + String(github_repo) + "/releases/latest/download/firmware.bin";
  
  logPrintf("Download from URL: %s\n", fwUrl.c_str());
    
  t_httpUpdate_return ret = httpUpdate.update(client, fwUrl);
  switch (ret) {
      case HTTP_UPDATE_FAILED:
          logPrintf("OTA Failed, err(%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
          break;
          
      case HTTP_UPDATE_NO_UPDATES:
          logPrintf("No .bin found");
          break;
          
      case HTTP_UPDATE_OK:
          logPrintf("OTA Ok, reboot...");
          execOtaReboot();
          break;
  }
}

void execOtaReboot(void)
{
  Wire.end(); 
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  delay(20);
  ESP.restart();
}


