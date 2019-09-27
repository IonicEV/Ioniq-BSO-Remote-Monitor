#ifndef WiFiELM327Client_h
#define WiFiELM327Client_h

#include <Arduino.h>
#include <WiFiClient.h>
#include "ELM327Client.h"

class WiFiELM327Client : public EML327Client {
  private:
  IPAddress _serverOBD;
  int _portOBD;
  bool _connected;
  
  public:
  ~WiFiELM327Client() {
    if (_elm327) {
      delete _elm327;
    }
  }
  WiFiELM327Client(IPAddress serverOBD, int portOBD) {
    _serverOBD = serverOBD;
    _portOBD = portOBD;
    _elm327 = new WiFiClient();
  }
  
  bool setup() {
    Serial.println("WiFiELM327Client setup");
  }
  bool connect() {
    Serial.println("WiFiELM327Client connect");
    Serial.print("Connecting to WiFi ELM327 "); Serial.print(_serverOBD.toString()); Serial.print(":"); Serial.println(_portOBD);
    if(_isTestELM327) { _connected = true; return true; }
    static_cast<WiFiClient*>(_elm327)->connect(_serverOBD, _portOBD);
    _connected =  static_cast<WiFiClient*>(_elm327)->connected();
    return _connected;
  }
  bool isConnected() {
    _connected =  _isTestELM327 ? _connected : static_cast<WiFiClient*>(_elm327)->connected();
    return _connected;
  }
  void stop() {
    _connected = false;
    Serial.println("Stopping to WiFi OBD");
    if(_isTestELM327) {  return; }
    static_cast<WiFiClient*>(_elm327)->stop();
  }
  void reset() {
    _connected = false;
    Serial.println("WiFiELM327Client reset");
    if(_isTestELM327) {  return; }
    static_cast<WiFiClient*>(_elm327)->stop();  
  }

};
#endif
