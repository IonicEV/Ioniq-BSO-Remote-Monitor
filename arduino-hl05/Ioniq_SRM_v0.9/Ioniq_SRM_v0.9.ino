/*
  Ioniq SRM (BOS Remote Monitor)
  Copyright (c) 2019 for WE Koyote ioniq->wekoyote at gmail.com. All rights reserved.

  https://github.com/Wekoyote/Ioniq-BSO-Remote-Monitor

  sudo esptool.py --baud 921600 --port /dev/tty.wchusbserial14130 write_flash -fm dio 0x00000 ~/Downloads/nodemcu_float_0.9.6-dev_20150704.bin
*/

#include <stdlib.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EasyDDNS.h>         // https://github.com/ayushsharma82/EasyDDNS
#include <PubSubClient.h>     // IMPORTANT: Modify #define MQTT_MAX_PACKET_SIZE 256 in PubSubClient.h file from library directory
#include "Config.h"
#include "Ioniq.h"

bool SynchronizeTime();
void stopLoop(char *message);
String getTimeDifference(int difference);
String getTimeStamp(int epochTime);
void telegramSend(char *telegramText);
char* createTelegram(char *dest, int len, Ioniq *ioniq, String &timeInitial, String &timeCurrent, String &totalTime);
void createMQTTmessage(char *dest, int len, Ioniq *ioniq);
void publishMQTTMessage(char *message);

// ***************************************************************************
// GLOBAL VARS
// ***************************************************************************
WiFiClient mqttConnect;
PubSubClient mqttClient(mqttConnect);
WiFiClientSecure secureClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, (NTP_DST ? NTP_ZONE * 3600 + 3600 : NTP_ZONE * 3600)); //Configure your NTP_ZONE in Config.h

#ifdef ENABLE_HELTEC_WIFI_Kit_8
OLED display(OLED_SDA, OLED_SCL);
#endif
char sendBuffer[SEND_BUF_LEN];
char recvBuffer[RECV_BUF_LEN];

#ifdef ENABLE_HC05
HC05EML327Client *clientOBD; // using default pin assignment for HC05
#else
WiFiELM327Client *clientOBD;
IPAddress serverOBD;
#endif

Ioniq *ioniq;

String clientId = "IoniqOBD-" + String(random(0xffff), HEX); //Client ID for server mqtt identification
String timeInitial = "";

int sleepTime = SLEEP_TIME;
int ResetCount = 0;
int timeInitialEpoch;

char bsoMsg[16];
char cecInitialMsg[16];
char cecDecimalMsg[16];

// ***************************************************************************
// SETUP
// ***************************************************************************
void setup()  {
  Serial.begin(115200);

  // ***************************************************************************
  // Setup: OLED Screen Wifi Kit_8
  // ***************************************************************************
#ifdef ENABLE_HELTEC_WIFI_Kit_8
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);   // turn D2 low to reset OLED
  delay(50);
  digitalWrite(OLED_RST, HIGH);   // while OLED is running, must set D2 in high
  display.begin();

  // Intro screen
  display.print("Ioniq SOC Remote", 0);
  display.print("     v ", 1);
  display.print(versionSoft, 1, 8);
  display.print("  by WE Koyote  ", 3);
  delay(4000);
  //display.clear();
#endif

  // Start WiFi
  WiFi.begin(WIFI_SSID, WIFI_PSW);
  while (WiFi.status() != WL_CONNECTED)  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  timeClient.begin();
  while(!SynchronizeTime()) {
    Serial.println("SynchronizeTime() failed");
    delay(10000);
  }
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

#ifdef ENABLE_HELTEC_WIFI_Kit_8
  display.print("Connecting....   ", 3);
#endif

// create selected elm327 client
#ifdef ENABLE_HC05
  #ifdef ENABLE_HELTEC_WIFI_Kit_8
  clientOBD = new HC05EML327Client(HC05_KIT8_txD, HC05_KIT8_rxD, HC05_KIT8_CMD, HC05_KIT8_RESET, HC05_KIT8_STATE);
  #else
  clientOBD = new HC05EML327Client();
  #endif
  clientOBD->setRemoteMacAddress(BT_MAC_ADDRESS);
#else
  if (!serverOBD.fromString(OBD_IP)) {
    stopLoop("Invalid WiFi OBD IP specified. Stopped!");
  }
  clientOBD = new WiFiELM327Client(serverOBD, OBD_PORT);
#endif
  // create Ionic with with selected elm327 client
  ioniq = new Ioniq(clientOBD, IONIQ_MODEL);

#ifdef ENABLE_TEST_ELM327 || ENABLE_TEST_OBD
  clientOBD->enableTestMode(ENABLE_TEST_ELM327, ENABLE_TEST_OBD);
  ioniq->isTestMode = ENABLE_TEST_ELM327 ||  ENABLE_TEST_OBD;
#endif

  delay(500);
  Serial.println("Setup OBD ");
  clientOBD->setup();
  Serial.println("Connect OBD ");
  while (!clientOBD->connect()) {
    Serial.println("Failed to connect OBD!");
    delay(5000);
  }
  Serial.println("OBD Connected");

#ifdef ENABLE_HELTEC_WIFI_Kit_8
  display.print("  OBD Connected  ", 3);
#endif

  delay(500);
  clientOBD->initialize();

#ifdef ENABLE_HELTEC_WIFI_Kit_8
  display.print("                ", 1);
#endif

  SynchronizeTime();

#ifdef ENABLE_DDNS
  EasyDDNS.service(DDNS_SERVICE);
  EasyDDNS.client(DDNS_DOMAIN, DDNS_USER, DDNS_PSW);
#endif
}

// ***************************************************************************
// LOOP
// ***************************************************************************
void loop() {

#ifdef ENABLE_DDNS
  EasyDDNS.update(DDNS_UPDATE_MS);
#endif
  SynchronizeTime();

  // Connecting to OBD
  Serial.println("");
  Serial.println("-------------------------------------");
  Serial.println("");

  if (!clientOBD->isConnected()) {
    clientOBD->connect();
  }

#ifdef ENABLE_HELTEC_WIFI_Kit_8
  display.print("OBD update      ", 2);
#endif

  Serial.println("======================");
  ioniq->update(); // get OBD status update
  // *******************************************************************************
  // The next 'if' I use to discard erroneous lines. If the content is useless
  // I do not enter, but if it is correct or it is in test mode, I continue forward
  // *******************************************************************************
  if (ioniq->getStatus() && ioniq->bsoDecimal > 10.00) {

    ResetCount = 0;
    // *******************************************************************************
    // Start the construction of the MQTT message
    // *******************************************************************************

    dtostrf(ioniq->bsoDecimal, 2, 1, bsoMsg);
    dtostrf(ioniq->cecDecimal, 2, 1, cecDecimalMsg);
    dtostrf(ioniq->cecInitial, 2, 1, cecInitialMsg);

#ifdef ENABLE_HELTEC_WIFI_Kit_8
    display.print(cecInitialMsg, 1, 0);
    display.print(cecDecimalMsg, 1, 8);
    display.print("Publish         ", 2);
    display.print("  Battery: ", 3);
    display.print(bsoMsg, 3, 11);
#endif
    createMQTTmessage(sendBuffer, MQTT_BUF_LEN, ioniq);
    publishMQTTMessage(sendBuffer);

    if (ioniq->getStatus() && ioniq->isCharging() && timeInitial == "") { // consume or charge battery? Surely there are better ways to do it. Sorry.
      timeClient.update();
      //Serial.println(getTimeStampString());
      timeInitial = (getTimeStamp(timeClient.getEpochTime()));
      timeInitialEpoch = (timeClient.getEpochTime());
    }

#ifdef ENABLE_TELEGRAM
    if (!ioniq->isCharging() && ioniq->notify) {

      if ((timeClient.getEpochTime() - timeInitialEpoch) > 30
          && ioniq->bsoInitial > 10 && ioniq->bsoInitial < ioniq->bsoDecimal
          && (ioniq->cecDecimal - ioniq->cecInitial) > 0) { //To avoid "bounces" is not sent if the load has been greater than 60 seconds
        timeClient.update();
        Serial.print("Preparing Telegram, lang: "); Serial.println(TELEGRAM_LANG_ESP ? "ESP" : "ENG");
        String currentTime = getTimeStamp(timeClient.getEpochTime());
        String totalTime = getTimeDifference(timeClient.getEpochTime() - timeInitialEpoch);
        createTelegram(sendBuffer, SEND_BUF_LEN, ioniq, timeInitial, currentTime, totalTime);
#ifdef ENABLE_HELTEC_WIFI_Kit_8
        delay(1000);
        display.print("Send Telegram   ", 2);
#endif
        telegramSend(sendBuffer);
      }
      ioniq->resetToCharged();
      timeInitial = "";
      timeInitialEpoch = 0;
    }
#endif

  } else {
    ResetCount = ResetCount + 1; // if the serial data is wrong...
    Serial.print("Reset control = ");
    Serial.println(ResetCount);
  }

  Serial.print("Sleeping ...");
#ifdef ENABLE_HELTEC_WIFI_Kit_8
  delay(1000);
  display.print("Sleeping ...    ", 2);
#endif
  delay(sleepTime);
  Serial.println("Wake up!");

  if (ResetCount >= 10) { // If data is corrupted or incorrect, restart ESP. Pending assigning a PIN to RESET.
    //ESP.reset();  // Not run in Wifi Kit 8 form Heltec
    //ESP.restart() // Not run in Wifi Kit 8 form Heltec
    ResetCount = 0;
    clientOBD->setup();
    clientOBD->reset();
    delay(5000);
    while (!clientOBD->connect()) {
      Serial.println("Failed to connect OBD!");
      delay(15000);
    }
    delay(500);
    clientOBD->initialize();
  }
}

// ***************************************************************************
//
//                    The different functions starts here
//
// ***************************************************************************
//
// ***************************************************************************
// Function builds MQTT message from Ioniq data
// ***************************************************************************
void createMQTTmessage(char *dest, int len, Ioniq *ioniq) {
  snprintf(dest, len, "{"
           "\"cec\":%.2f,"
           "\"bso\":%.2f,"
           "\"i\":%.2f,"
           "\"v\":%.2f,"
           "\"p\":%.2f,"
           "\"t\":%.2f,"
           "\"tu\":%.2f,"
           "\"tl\":%.2f,"
           "\"ch\":%d,"
           "\"end\":%d,"
           "\"f\":%d,"
           "\"k\":%d"
           "}"
           ,
           (ioniq->cecDecimal - ioniq->cecInitial),
           ioniq->bsoDecimal,
           ioniq->currentDecimal,
           ioniq->voltageDecimal,
           ioniq->powerDecimal,
           ioniq->tempDecimal,
           ioniq->tempMaxDecimal,
           ioniq->tempMinDecimal,
           ioniq->intCharger,
           ioniq->endCharge,
           ioniq->intFan,
           ioniq->intKms
          );
}

// ***************************************************************************
// Function to publish MQTT message with Ioniq data
// ***************************************************************************
void publishMQTTMessage(char *message) {
  Serial.print("MQTT Msg:"); Serial.print(strlen(message)); Serial.print("->"); Serial.println(message);
  Serial.println("Connecting to  MQTT...");
  if (mqttClient.connected() || mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PSW)) { // Attempt to connect to MQTT server
    Serial.println("Connected to MQTT");
  } else {
    Serial.print("Failed, rc="); Serial.print(mqttClient.state()); Serial.println(" resuming in 5 seconds");
    delay(5000); // Wait 5 seconds before retrying
  }

  if (mqttClient.connected() || mqttClient.connect(clientId.c_str())) {
    Serial.println("Publishing to MQTT");
    if (!mqttClient.publish(MQTT_NODE, sendBuffer, true)) {
      Serial.println("MQTT publish error!!");
    }
  } else {
    Serial.println("MQTT connect failed!");
  }
}

// ***************************************************************************
// Function builds Telegram message from Ioniq data
// ***************************************************************************
char* createTelegram(char *dest, int len, Ioniq *ioniq, String &timeInitial, String &timeCurrent, String &totalTime) {
  if (TELEGRAM_LANG_ESP) {
    snprintf(dest, len,
             "GET /%s/sendMessage?parse_mode=HTML&chat_id=%s&text="
             "<b>INFORME DE CARGA DE</b>%%0A"
             "<b>%s</b>%%0A"
             "<b>=======================</b>%%0A"
             "<b>Hora inicio:</b> %s %%0A"
             "<b>Hora final:</b> %s %%0A"
             "<b>Tiempo:</b> %s h %%0A"
             "--------------------------------- %%0A"
             "<b>CEC inicio:</b> %.2f kWh %%0A"
             "<b>CEC final:</b> %.2f kWh %%0A"
             "<b>Energía cargada:</b> %.2f kWh %%0A"
             "---------------------------------%%0A"
             "<b>Carga inicio:</b> %.2f  %% %%0A"
             "<b>Carga final:</b> %.2f  %% %%0A"
             "<b>Carga realizada:</b> %.2f %% %%0A"
             "---------------------------------%%0A"
             "<b>Temperatura máxima:</b>  %.2fº %%0A"
             "<b>Temperatura mínima:</b>  %.2fº %%0A"
             "<b>Temperatura media:</b>  %.2fº %%0A"
             "<b>Ventilador:</b> %s %%0A"
             "---------------------------------%%0A"
             "Ioniq SOC Remonte Monitor %%0Aby <b>WE Koyote</b> %%0A%%0A"
             "\n\r",
             TELEGRAM_BOT_TOKEN , TELEGRAM_CHAT_ID,
             ioniq->getVehicleId().c_str(),
             timeInitial.c_str(),
             timeCurrent.c_str(),
             totalTime.c_str(),
             ioniq->cecInitial,
             ioniq->cecDecimal,
             (ioniq->cecDecimal - ioniq->cecInitial),
             ioniq->bsoInitial,
             ioniq->bsoDecimal,
             (ioniq->bsoDecimal - ioniq->bsoInitial),
             ioniq->tempMaxDecimal,
             ioniq->tempMinDecimal,
             ((ioniq->tempMaxDecimal + ioniq->tempMinDecimal) / 2),
             (ioniq->intFan ? "Arrancó" : "No arrancó")
            );
  } else {
    snprintf(dest, len,
             "GET /%s/sendMessage?parse_mode=HTML&chat_id=%s&text="
             "<b>CHARGE REPORT</b>%%0A"
             "<b>%s</b>%%0A"
             "<b>=======================</b>%%0A%%0A"
             "<b>Start Time:</b> %s %%0A"
             "<b>Final Time:</b> %s %%0A"
             "<b>Time:</b> %s h %%0A"
             "---------------------------------%%0A"
             "<b>CEC Start:</b> %.2f kWh %%0A"
             "<b>CEC Final:</b> %.2f kWh %%0A"
             "<b>Charged energy:</b> %.2f kWh %%0A"
             "---------------------------------%%0A"
             "<b>BSO Start:</b> %.2f %% %%0A"
             "<b>BSO Final:</b> %.2f %% %%0A"
             "<b>Charged:</b> %.2f %% %%0A"
             "---------------------------------%%0A"
             "<b>Max Temperature:</b>  %.2fº %%0A"
             "<b>Min Temperature:</b>  %.2fº %%0A"
             "<b>Average Temperature:</b>  %.2fº %%0A"
             "<b>Battery Fan:</b> %s %%0A"
             "---------------------------------%%0A"
             "Ioniq SOC Remonte Monitor %%0AAby <b>WE Koyote</b>%%0A%%0A"
             "\n\r",
             TELEGRAM_BOT_TOKEN , TELEGRAM_CHAT_ID,
             ioniq->getVehicleId().c_str(),
             timeInitial.c_str(),
             timeCurrent.c_str(),
             totalTime.c_str(),
             ioniq->cecInitial,
             ioniq->cecDecimal,
             (ioniq->cecDecimal - ioniq->cecInitial),
             ioniq->bsoInitial,
             ioniq->bsoDecimal,
             (ioniq->bsoDecimal - ioniq->bsoInitial),
             ioniq->tempMaxDecimal,
             ioniq->tempMinDecimal,
             ((ioniq->tempMaxDecimal + ioniq->tempMinDecimal) / 2),
             (ioniq->intFan ? "Active" : "Not Active")
            );
  }
  return dest;
}

// ***************************************************************************
// Function for send report via telegram
// ***************************************************************************
//
void telegramSend(char *telegramText) {
  int retry = 2;
  do {
    Serial.print("Sending Telegram..."); Serial.println(retry);
    if (secureClient.connect(TELEGRAM_SERVER, 443)) {
      Serial.print(strlen(telegramText));
      Serial.print(":");
      Serial.println(telegramText);
      secureClient.println(telegramText);
      delay(1000);
      secureClient.setTimeout(5000);
      int recv = secureClient.readBytesUntil(',', recvBuffer, RECV_BUF_LEN);
      if (recv == 0 && strcmp(recvBuffer, "{\"ok\":true") != 0) {
        Serial.println("Send Telegram Error!! "); Serial.println(recvBuffer);
      }
      secureClient.stop();
      retry = 0;
    } else {
      Serial.println("Connect to Telegram FAILED!!");
      delay(5000);
    }
  } while(--retry > 0);
}

// ***************************************************************************
// Function time every  NTP_UPDATE_INTERNVAL milliSeconds
// ***************************************************************************
bool SynchronizeTime() {
  static unsigned long timer;
  if (!timer || millis() > (timer + NTP_UPDATE_INTERNVAL)) {
    timer = millis();
    Serial.println("Synchronizing date and time..");
    if (!timeClient.update()) {
      return false;
    }
    Serial.print("Date & Time: ");
    Serial.println(getTimeStamp(timeClient.getEpochTime()));
  }
  return true;
}
// ***************************************************************************
// Return formated date/time
// ***************************************************************************
// https://github.com/arduino-libraries/NTPClient/issues/36
//
String getTimeStamp(int epochTime) {
  char timeBuffer[25];
  time_t rawtime = epochTime;
  struct tm * ti;
  ti = localtime (&rawtime);

  snprintf(timeBuffer, 25, "%02d-%02d-%02d %02d:%02d:%02d",
           ti->tm_year + 1900,
           ti->tm_mon + 1,
           ti->tm_mday,
           ti->tm_hour,
           ti->tm_min,
           ti->tm_sec
          );
  return timeBuffer;
}

// ***************************************************************************
// Return difference between two date/time
// ***************************************************************************
//
String getTimeDifference(int difference) {
  char timeBuffer[25];
  time_t rawtime = difference;
  struct tm * ti;
  ti = localtime (&rawtime);

  snprintf(timeBuffer, 25, "%d:%02d:%02d",
           ti->tm_hour + ((ti->tm_mday - 1) * 24), // hopefully it charges in less than a day ;)
           ti->tm_min,
           ti->tm_sec
          );
  return timeBuffer;
}

void stopLoop(char *message) {
  Serial.println(message);
#ifdef ENABLE_HELTEC_WIFI_Kit_8
  display.print(message, 3);
#endif
  while (1) {
    delay(1000);
  }
}
