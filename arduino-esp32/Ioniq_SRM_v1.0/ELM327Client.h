#ifndef ELM327client_h
#define ELM327client_h
#include <Stream.h>

#define ELM327_TERM_CHAR '>'         //OBD terminator character
#define ELM327_BUF_LEN 368

enum ELM327_OBD_Status {
  ELM327_OBD_NO_ERROR,
  ELM327_OBD_ERROR_CAN,
  ELM327_OBD_ERROR_NO_DATA,
  ELM327_OBD_ERROR_ECU_NORESPONSE,
  ELM327_OBD_ERROR_ECU_SEARCHING
};

class EML327Client {
  protected:
  Stream *_elm327;
  bool _isTestELM327;
  bool _isTestOBD;
  bool _isError;
  ELM327_OBD_Status obdStatus;
  bool executeATCommand(char * atCommand) {
    //For clean serial buffer
    Serial.print("Sending ELM327 command: "); Serial.println(atCommand);
    if (_isTestELM327) {  return true; }
    flush();
    _elm327->print(atCommand); _elm327->print("\r\n");
    int recvCount = _elm327->readBytesUntil(ELM327_TERM_CHAR, elm327Buffer, ELM327_BUF_LEN - 1);
    if (recvCount > 0) {
      elm327Buffer[recvCount] = 0;
      Serial.println(clean_str(elm327Buffer));
      return true;
    }
    Serial.println("Error sending ELM327 AT command!");
    return false;
  }
  
  public:
  char elm327Buffer[ELM327_BUF_LEN];

  EML327Client() {
    obdStatus = ELM327_OBD_NO_ERROR;
  }
  void enableTestMode(bool testELM327, bool testBOD) {
    _isTestELM327 = testELM327;
    _isTestOBD = testBOD;
  }
  bool isError() {
    return _isError;
  }  
  virtual bool setup() = 0;
  virtual bool connect() = 0;
  virtual bool isConnected() = 0;
  virtual void flush() {
    _elm327->flush();
  };
  virtual void stop() = 0;
  virtual void reset() = 0;
  //ELM327 specific
  bool initialize() {
    Serial.println("Initializing ELM327 OBD");
    boolean ok = false;
    if(_elm327 == NULL) {
      Serial.println("Stream is null pointer");
      return ok;
    }
    ok = executeATCommand("ATZ");      // RESET OBD
    ok = executeATCommand("ATE0");     // ECHO OFF
    ok = executeATCommand("ATL1");     // LF OFF
    ok = executeATCommand("ATH0");     // Headers OFF
    ok = executeATCommand("ATST62");   // Timeout to 62 x 4 msecs
    ok = executeATCommand("ATSH 7E4"); // Filter for 7E4
    Serial.println();
    return ok;
  }
  int executeOBDCommand(char * obdCommand) {
    int recv;
    //For clean serial buffer
    Serial.println("======================");
    Serial.print("Sending OBD command: "); Serial.println(obdCommand);
    if (_isTestOBD) {  elm327Buffer[0] = 0; return 0; }
    int retry = 5;
    bool ok = false;
    do {
      flush();
      _elm327->print(obdCommand); _elm327->print("\r\n");
      _elm327->setTimeout(5000);
      recv = _elm327->readBytesUntil(ELM327_TERM_CHAR, elm327Buffer, ELM327_BUF_LEN -1);
      elm327Buffer[recv] = 0; 
      // check for errors 
      if (strncmp(elm327Buffer,"CAN ERROR", 9) == 0) { //If ECU is OFF
        obdStatus = ELM327_OBD_ERROR_CAN;
        Serial.print ("ERROR: "); Serial.println (elm327Buffer);
        elm327Buffer[0] = 0;
      } else if (strncmp(elm327Buffer, "NO DATA", 7) == 0) { //If OBD device not respond
        obdStatus = ELM327_OBD_ERROR_NO_DATA;
        Serial.print ("ERROR: "); Serial.println (elm327Buffer);
        elm327Buffer[0] = 0;
      } else if (strncmp(elm327Buffer, "UNABLE", 6) == 0) { //If OBD device not respond
        obdStatus = ELM327_OBD_ERROR_ECU_NORESPONSE;
        Serial.print ("ERROR: "); Serial.println (elm327Buffer);
        elm327Buffer[0] = 0;
      } else if (strncmp(elm327Buffer, "SEARCHING", 9) == 0) { //if the serial does not respond as expected
        obdStatus = ELM327_OBD_ERROR_ECU_SEARCHING;
        Serial.print ("ERROR: "); Serial.println (elm327Buffer);
        elm327Buffer[0] = 0;
      }
      if (elm327Buffer[0]) { ok = true; }
      else { delay(1000); }
    } while (!ok && --retry > 0);
    if (ok) {
      Serial.print("OBD response: "); Serial.println(elm327Buffer);
    }
    return recv;
  }

  char *clean_str(char *str)
  {
    char *p, *d; 
    p = d = str;
    if (!str) {
      return str;
    }
    while (char ch = *p++) {
      if (ch != '\r' && ch != '\n' ) {
        *d++ = ch;
      } 
    }
    *d = 0;
    return str;
  }

};

#endif
