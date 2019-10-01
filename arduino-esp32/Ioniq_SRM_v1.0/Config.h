#define ENABLE_TELEGRAM;        //Disconnect if you do not want notifications by Telegram bot
#define ENABLE_DDNS;            //Disconnect if not use dynamic DNS
#define ENABLE_TEST_ELM327  1   //If enabled, ELM327 interaction is bypassed
#define ENABLE_TEST_OBD     1   //If enabled, OBD communication is bypassed
#define IONIQ_MODEL IONIQ_EV17  // Refer to IONIQ_Model enum in Ionic.h to pick your model
//#define ENABLE_HELTEC_WIFI_Kit_32  //If defined, enable for compile HELTEC_WIFI_Kit_32

#ifdef ENABLE_HELTEC_WIFI_Kit_32
/*
Install "ESP8266-OLED Display Library" by Klar Systems
You would need to make adjustments in the OLED.cpp init_OLED() for specific screen resolution
The following config changes would work on Heltec 8266 and ESP32 variants 128x32 and 128x64
OLED.cpp
find line: sendcommand(0x3F);
replace with: sendcommand(0x1F);

find line: sendcommand(0x12);;
replace with: sendcommand(0x02);

library.properties
find line: architectures=esp8266
replace with: architectures=esp8266,esp32
*/
#include <Wire.h>
#include <OLED.h>
#define OLED_RST 16
#define OLED_SDA 4
#define OLED_SCL 15
#endif

#define MQTT_BUF_LEN 256
#define RECV_BUF_LEN 128
#define SEND_BUF_LEN 1024

#include "ESP32ELM327Client.h"
#define LOCAL_NAME "ESP32KIT321" //"ESP32DEV1" //"ESP32KIT321"  //"ESP32KIT322"
//#define ADDRESS  "00:1D:A5:02:C3:22" // ESP32DEV1
#define ADDRESS  "AA:BB:CC:11:22:33" // ESP32KIT321
//#define ADDRESS  "00:1D:A5:1E:DD:12" // ESP32KIT322
//uint8_t address[6]  = {0x00, 0x1D, 0xA5, 0x02, 0xC3, 0x22};
//uint8_t address[6]  = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};

// ***************************************************************************
// IMPORTANT: Enable wifi and OBD fot test or work in Ioniq
// ***************************************************************************
#ifdef ENABLE_TEST_ELM327  //Enable wifi for test in your LAN

#define SLEEP_TIME        5000  //Sleep time between OBD requests
#define WIFI_SSID  "myHome"
#define WIFI_PSW   "myHome-psw"

#else  //Enable wifi and IP for OBD in your Ioniq

#ifdef ENABLE_TEST_OBD
#define SLEEP_TIME        5000  //Sleep time between OBD requests
#else
#define SLEEP_TIME        15000  //Sleep time between OBD requests
#endif

#define WIFI_SSID  "YourWIFI_SSIDIoniq"
#define WIFI_PSW = "WIFI_PSWWifiIoniq"

#endif

// ***************************************************************************
// Config server mqttt cloudmqtt.com. Support persistence data.
// ***************************************************************************
#define MQTT_SERVER  "soldier.cloudmqtt.com"  // Your server in CloudMQTT
#define MQTT_PORT     18044                   // Your port NOT SSL
#define MQTT_USER     "ioniq"                 // Your user in CloudMQTT
#define MQTT_PSW      "user-psw"              // Your WIFI_PSW in CloudMQTT
#define MQTT_NODE     "ioniq/bso"             // Your topiq in CloudMQTT


// ***************************************************************************
// Config Telegram - https://core.telegram.org/bots#3-how-do-i-create-a-bot
// ***************************************************************************
#define TELEGRAM_SERVER  "api.telegram.org"
#define TELEGRAM_LANG_ESP 0 // 1 = ESP= Spanish , otherwise  ENG = English
#define TELEGRAM_BOT_TOKEN "bot0987654321:BLA5fqp8ayE8np6YdJRQKBLzQV8eIDnTBLA"  //token format is botXXXXXXXXXXXXXXXXXXXXXXXXXXX
#define TELEGRAM_CHAT_ID  "01234567890" // Chat_id



// ***************************************************************************
// Config DDNS
// ***************************************************************************
#define DDNS_SERVICE "noip" // Enter your DDNS Service Name - "duckdns" / "noip" / "dyndns" / "dynu" / "enom".
#define DDNS_DOMAIN  "ioniq19.ddns.net"
#define DDNS_USER    "myuser-name@yahoo.com"
#define DDNS_PSW     "my-psw"
#define DDNS_UPDATE_MS 100000 // Check for New Ip Every 100 Seconds.

// ***************************************************************************
// Config NTPClient
// ***************************************************************************
#define NTP_SERVER "north-america.pool.ntp.org" // your reional NTP server pool
#define NTP_ZONE  -5       // Your time zone
#define NTP_DST   1        // False for winter time, true for summer time.
#define NTP_UPDATE_INTERNVAL 300000 // Set update interval to 5 mins
// ***************************************************************************
// Config variables
// ***************************************************************************
#define versionSoft "1.0"
