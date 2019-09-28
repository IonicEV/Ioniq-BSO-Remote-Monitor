#ifndef ESP32ELM327Client_h
#define ESP32ELM327Client_h

#include "BluetoothSerial.h"
#include "ELM327Client.h"

class ESP32ELM327Client : public EML327Client {
  private:
  uint8_t _remote_address[ESP_BD_ADDR_LEN];
  bool _connected;
  String _local_name;

  char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
  {
    if (bda == NULL || str == NULL || size < 18) {
      return NULL;
    }
  
    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
  }

  public:
  ~ESP32ELM327Client() {
    if (_elm327) {
      delete _elm327;
    }
  }
  ESP32ELM327Client(char *address, String localName=String()) {
    sscanf(address, "%02x:%02x:%02x:%02x:%02x:%02x",
                &_remote_address[0], &_remote_address[1], &_remote_address[2],
                &_remote_address[3], &_remote_address[4], &_remote_address[5]);
    _elm327 = new BluetoothSerial();
    if (localName.length()){
        _local_name = localName;
    }
  }
  ESP32ELM327Client(uint8_t address[], String localName=String()) {
    memcpy(_remote_address, address, ESP_BD_ADDR_LEN);
    _elm327 = new BluetoothSerial();
     if (localName.length()){
        _local_name = localName;
    }
 }
  
  bool setup() {
    Serial.println("ESP32ELM327Client setup");
    static_cast<BluetoothSerial*>(_elm327)->begin(_local_name, true);
  }
  bool connect() {
    char bda_str[18];
    Serial.println("ESP32ELM327Client connect");
    Serial.print("Connecting to BT ELM327 "); Serial.println(bda2str(_remote_address, bda_str, 18));
    if(_isTestELM327) { _connected = true; return true; }
    static_cast<BluetoothSerial*>(_elm327)->connect(_remote_address);
    _connected =  static_cast<BluetoothSerial*>(_elm327)->connected();
    if (!_connected) {
      //try to remove paring info and repeat
      static_cast<BluetoothSerial*>(_elm327)->remove_bond_device(_remote_address);
      static_cast<BluetoothSerial*>(_elm327)->connect(_remote_address);
      _connected =  static_cast<BluetoothSerial*>(_elm327)->connected();
    }
    return _connected;
  }
  bool isConnected() {
    _connected =  _isTestELM327 ? _connected : static_cast<BluetoothSerial*>(_elm327)->connected();
    return _connected;
  }
  void stop() {
    _connected = false;
    Serial.println("ESP32ELM327Client stop");
    if(_isTestELM327) {  return; }
    static_cast<BluetoothSerial*>(_elm327)->disconnect();
  }
  void reset() {
    _connected = false;
    Serial.println("ESP32ELM327Client reset");
    if(_isTestELM327) {  return; }
    static_cast<BluetoothSerial*>(_elm327)->disconnect();  
  }

};
#endif
