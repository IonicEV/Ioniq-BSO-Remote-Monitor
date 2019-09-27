#ifndef HC05ELM327Client_h
#define HC05ELM327Client_h

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ELM327Client.h"

#define HC05_txD   15  //Arduino pin connected to Rx of HC-05
#define HC05_rxD   13  //Arduino pin connected to Tx of HC-05
#define HC05_CMD   14  //Arduino pin connected to CMD (34) of HC-05 (enter AT Mode with HIGH)
#define HC05_RESET 16  //Arduino pin connected to Reset of HC-05 (reset with LOW)
#define HC05_STATE 12  //Arduino pin connected to pin 24 status of HC-05 (Low not connected, HIGH connected)

#define HC05_KIT8_txD    0  //Arduino pin connected to Rx of HC-05
#define HC05_KIT8_rxD   15  //Arduino pin connected to Tx of HC-05
#define HC05_KIT8_CMD   14  //Arduino pin connected to CMD (34) of HC-05 (enter AT Mode with HIGH)
#define HC05_KIT8_RESET 12  //Arduino pin connected to Reset of HC-05 (reset with LOW)
#define HC05_KIT8_STATE 13  //Arduino pin connected to pin 24 status of HC-05 (Low not connected, HIGH connected)

#define HC05_CMD_RETRIES 5      //Number of retries for each Bluetooth AT command in case of not responde with OK

class HC05EML327Client : public EML327Client {
  private:
  
  int _tx, _rx, _pio11, _rst, _sts;
  String _remoteMacAddress;
  bool _connected;
  
  bool setupBTConnection();
  void resetBT();
  void enterBT_ComMode();
  void enterBT_ATMode();
  void sendBT_ATCommand(const char *command, bool ignoreError = false, int cmd_retries = HC05_CMD_RETRIES);
  bool waitForConnection();

  public:
  ~HC05EML327Client() {
    if (_elm327) {
      delete _elm327;
    }
  }
  HC05EML327Client();
  HC05EML327Client(int Tx, int Rx, int pio11, int rst, int sts);
  void setRemoteMacAddress(char * remoteMacAddress);
  bool setup();
  bool connect();
  bool isConnected(); 
  void stop();
  void reset();
  
};
#endif
