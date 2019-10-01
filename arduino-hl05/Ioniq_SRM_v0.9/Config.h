#define ENABLE_TELEGRAM;        //Disconnect if you do not want notifications by Telegram bot
#define ENABLE_DDNS;            //Disconnect if not use dynamic DNS
#define ENABLE_TEST_ELM327  1   //If enabled, ELM327 interaction is bypassed
#define ENABLE_TEST_OBD     1   //If enabled, OBD communication is bypassed
#define ENABLE_HC05             //If enabled, BT connection is used instead of WiFi
#define IONIQ_MODEL IONIQ_EV17  // Refer to IONIQ_Model enum in Ionic.h to pick your model
//#define ENABLE_HELTEC_WIFI_Kit_8  //If defined, enable for compile HELTEC_WIFI_Kit_8

#ifdef ENABLE_HELTEC_WIFI_Kit_8
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
replace with: architectures=esp8266
*/
#include <Wire.h>
#include <OLED.h>
#define OLED_RST 16               //Arduino pin D0
#define OLED_SDA 4                //Arduino pin D2
#define OLED_SCL 5                //Arduino pin D1
/*
  HELTEC WIFI_Kit_8's OLED connection:
  SDA -- GPIO4 -- D2
  SCL -- GPIO5 -- D1
  RST -- GPIO16 -- D0
*/
#endif

#define MQTT_BUF_LEN 256
#define RECV_BUF_LEN 128
#define SEND_BUF_LEN 1024

#ifdef ENABLE_HC05
#include "HC05ELM327Client.h"
#define BT_MAC_ADDRESS "AABB,CC,112233" //"B33B,10,0BA8DD" //"1D,A5,1EDD12"
#else
#include "WiFiELM327Client.h"
#endif

// ***************************************************************************
// IMPORTANT: Enable wifi and OBD fot test or work in Ioniq
// ***************************************************************************
#ifdef ENABLE_TEST_ELM327  //Enable wifi for test in your LAN

#define SLEEP_TIME        5000  //Sleep time between OBD requests
#define WIFI_SSID  "mySSID"
#define WIFI_PSW "mypws"
#define OBD_IP    "192.168.1.200"  //IP address OBD device
#define OBD_PORT  2323 // Port OBD device

#else  //Enable wifi and IP for OBD in your Ioniq

#ifdef ENABLE_TEST_OBD
#define SLEEP_TIME        5000  //Sleep time between OBD requests
#else
#define SLEEP_TIME        15000  //Sleep time between OBD requests
#endif

#define WIFI_SSID  "YourWIFI_SSIDIoniq"
#define WIFI_PSW = "WIFI_PSWWifiIoniq"
#define OBD_IP    "192.168.1.200"  //IP address OBD device
#define OBD_PORT 23  // Port OBD device

#endif

// ***************************************************************************
// Config server mqttt cloudmqtt.com. Support persistence data.
// ***************************************************************************
#define MQTT_SERVER  "soldier.cloudmqtt.com"  // Your server in CloudMQTT
#define MQTT_PORT     18044                   // Your port NOT SSL
#define MQTT_USER     "ioniq"                 // Your user in CloudMQTT
#define MQTT_PSW      "mypws"            // Your WIFI_PSW in CloudMQTT
#define MQTT_NODE     "ioniq/bso"             // Your topiq in CloudMQTT


// ***************************************************************************
// Config Telegram - https://core.telegram.org/bots#3-how-do-i-create-a-bot
// ***************************************************************************
#define TELEGRAM_SERVER  "api.telegram.org"
#define TELEGRAM_LANG_ESP 0 // 1 = ESP= Spanish , otherwise  ENG = English
#define TELEGRAM_BOT_TOKEN "bot0987654321:BLA5fqp8ayE8np6YdJRQKBLzQV8eIDnTbLA"  //token format is botXXXXXXXXXXXXXXXXXXXXXXXXXXX
#define TELEGRAM_CHAT_ID  "0123456789" // Chat_id



// ***************************************************************************
// Config DDNS
// ***************************************************************************
#define DDNS_SERVICE "noip" // Enter your DDNS Service Name - "duckdns" / "noip" / "dyndns" / "dynu" / "enom".
#define DDNS_DOMAIN  "ioniq19.ddns.net"
#define DDNS_USER    "myuser@yahoo.com"
#define DDNS_PSW     "mypsw"
#define DDNS_UPDATE_MS 10000 // Check for New Ip Every 10 Seconds.

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
#define versionSoft "0.9"
