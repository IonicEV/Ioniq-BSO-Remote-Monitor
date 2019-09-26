#define ENABLE_TELEGRAM;        //Disconnect if you do not want notifications by Telegram bot
#define ENABLE_DDNS;            //Disconnect if not use dynamic DNS
#define ENABLE_TEST_ELM327  1   //If enabled, ELM327 interaction is bypassed
#define ENABLE_TEST_OBD     1   //If enabled, OBD communication is bypassed
#define IONIQ_MODEL IONIQ_EV17  // Refer to IONIQ_Model enum in Ionic.h to pick your model

#include "ESP32ELM327Client.h"
#define LOCAL_NAME "ESP32DEV1" // host_name and BT local_name for your ESP32
#define ADDRESS  "00:1D:A5:02:C3:22"  //remote BT OBDII bluetooth address (you can find it by paiting your OBDII with Window sand then look at the connection properties)

// buffer length used by the app.
#define MQTT_BUF_LEN 256
#define RECV_BUF_LEN 128
#define SEND_BUF_LEN 1024



// ***************************************************************************
// IMPORTANT: Enable wifi and OBD fot test or work in Ioniq
// ***************************************************************************
#ifdef ENABLE_TEST_ELM327  /Enable wifi for test in your LAN

#define SLEEP_TIME 5000  //Sllep time between OBD requests
#define WIFI_SSID  "myHome"
#define WIFI_PSW   "myHome-psw"

#else  //Enable wifi and IP for OBD in your Ioniq

#define SLEEP_TIME        15000  //Sllep time between OBD requests
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
