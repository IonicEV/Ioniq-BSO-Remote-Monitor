#include "HC05ELM327Client.h"

HC05EML327Client::HC05EML327Client() : EML327Client() {
  _tx =    HC05_txD;
  _rx =    HC05_rxD;
  _pio11 = HC05_CMD;
  _rst =   HC05_RESET; 
  _sts =   HC05_STATE;  
  _elm327 = new SoftwareSerial(_rx, _tx);
}

HC05EML327Client::HC05EML327Client(int Tx, int Rx, int pio11, int rst, int sts) : EML327Client() {
  _tx =    Tx;
  _rx =    Rx;
  _pio11 = pio11;
  _rst =   rst;
  _sts =   sts;
  _elm327 = new SoftwareSerial(_rx, _tx);
}

void HC05EML327Client::setRemoteMacAddress(char * remoteMacAddress) {
  _remoteMacAddress = remoteMacAddress; 
}

bool HC05EML327Client::setup() {
  _connected = false;
  Serial.println("HC05EML327Client setup");
  pinMode(_sts, INPUT);
  pinMode(_rx, INPUT);
  pinMode(_tx, OUTPUT);
  pinMode(_pio11, OUTPUT);
  pinMode(_rst, OUTPUT);
  
  digitalWrite(_pio11, LOW);  //Set HC-05 to Com mode
  digitalWrite(_rst, HIGH);   //HC-05 no RESET  
  static_cast<SoftwareSerial*>(_elm327)->begin(38400); //default communication baud rate of HC-05 is 38400
}

bool HC05EML327Client::waitForConnection() {
  Serial.print("Waiting for HC05EML327Client connection: ");
  int retries = 0;
  while (digitalRead(_sts) != HIGH && retries < HC05_CMD_RETRIES) {
    retries++;
    delay(5000);
    Serial.print(".");
  }
  _connected = digitalRead(_sts) == HIGH;
  Serial.println(_connected ? " success" : " timeout");
  return _connected;
}

bool HC05EML327Client::connect() {
  Serial.println("HC05EML327Client connect");
  resetBT(); // need to reset it first after boot to get it in working state.
  waitForConnection();
  if (_connected) {
    return _connected;
  }
  
  Serial.println("Re-configuring BT OBD " + _remoteMacAddress);
  _connected = setupBTConnection();
  
   //confirm connection
  waitForConnection();
  if (_connected) {
    Serial.println("Connected to BT OBD successfully.");
  } else {
    Serial.println("Failed to confirm connection to BT OBD " + _remoteMacAddress);
  }  
  return _connected;
};

bool HC05EML327Client::isConnected() {
  _connected =  _isTestELM327 ? _connected : digitalRead(_sts) == HIGH;;
  return _connected;
};

void HC05EML327Client::stop() {
  Serial.println("HC05EML327Client stop");
  static_cast<SoftwareSerial*>(_elm327)->end();
};

void HC05EML327Client::reset() {
  Serial.println("HC05EML327Client reset");
  _connected = false;
  resetBT();
};

//----------------------------------------------------------//
//-----------start of bluetooth connection------------------//
bool HC05EML327Client::setupBTConnection() {
  _isError= false;                  //set bluetooth error flag to false
  Serial.println("Configuring remote BT ELM327 addr: " + _remoteMacAddress + " ...");
 
  enterBT_ATMode();                          //enter HC-05 AT mode
  delay(500);

  sendBT_ATCommand("RESET");                  // send to HC-05 RESET
  delay(2000);
  sendBT_ATCommand("ORGL");                   // Reset to defaults: Slave mode, pin code :1234, device name: H-C-2010-06-01 ,Baud 38400bits/s
  sendBT_ATCommand("VERSION?");
  sendBT_ATCommand("UART?");
  sendBT_ATCommand("ADDR?");
  sendBT_ATCommand("CLASS?");
  sendBT_ATCommand("PSWD?");                  // show bind pin code
  sendBT_ATCommand("ROLE=1");                 // send ROLE=1, set role to master
  sendBT_ATCommand("CMODE=0");                // send CMODE=0, set connection mode to specific address
  sendBT_ATCommand("RMAAD");                  // remove all connected device
  sendBT_ATCommand("DISC", true, 1);          // disconnect (it may connect during initialization)
  delay(1000);
  sendBT_ATCommand(("RNAME?" + _remoteMacAddress).c_str(), true, 2); // remote device name
  delay(1000);
  sendBT_ATCommand("INIT");                   //send INIT, Loads SPP library
  delay(1000);
  sendBT_ATCommand(("BIND=" + _remoteMacAddress).c_str());
  delay(1000);
  sendBT_ATCommand(("PAIR=" + _remoteMacAddress + ",20").c_str(), true, 1);    //send PAIR, pair with OBD address would fail if no psw is set on OBD
  delay(1000);
  sendBT_ATCommand(("LINK=" + _remoteMacAddress).c_str());    //send LINK, link with OBD address
  delay(1000);
  enterBT_ComMode();                          //enter HC-05 comunication mode
  delay(500);
  Serial.println("Configured remote BT OBD successfully.");
  return !_isError;
}

//----------------------------------------------------------//
//------------------RESET of HC-05--------------------------//
//-------set RESET pin of HC-05 to LOW for 2 secs-----------//
void HC05EML327Client::resetBT() {
  Serial.println("Resetting HC-05");
  if(_isTestELM327) {  return; }
  digitalWrite(_rst, LOW);
  delay(2000);
  digitalWrite(_rst, HIGH);
  delay(2000);
  Serial.println("Reset HC-05 complete");
}

//----------------------------------------------------------//
//--------Enter HC-05 bluetooth moduel command mode---------//
//-------------set HC-05 mode pin to LOW--------------------//
void HC05EML327Client::enterBT_ComMode() {
  Serial.println("Entering HC-05 Com mode");
  if(_isTestELM327) {  return; }
  delay(500);
  digitalWrite(_pio11, LOW);
  delay(500);
  static_cast<SoftwareSerial*>(_elm327)->begin(38400); //default communication baud rate of HC-05 is 38400
}

//----------------------------------------------------------//
//----------Enter HC-05 bluetooth moduel AT mode------------//
//-------------set HC-05 mode pin to HIGH-------------------//
void HC05EML327Client::enterBT_ATMode()
{
  Serial.println("Entering HC-05 AT mode");
  if(_isTestELM327) {  return; }
  delay(500);
  digitalWrite(_pio11, HIGH);
  delay(500);
  static_cast<SoftwareSerial*>(_elm327)->begin(38400);//HC-05 AT mode baud rate is 38400
}

//----------------------------------------------------------//
//----------Send HC-05 bluetooth module AT comman-----------//
void HC05EML327Client::sendBT_ATCommand(const char *command, bool ignoreError, int cmd_retries) {
  int retries;
  bool OK_flag, ERROR_flag, FAIL_flag;
  
  Serial.print("Sending HC-05 AT command: "); Serial.println(command); 
  if(_isTestELM327) {  return; }
  _elm327->flush();
  if (!_isError) {                                 //if no bluetooth connection error
    retries = 0;
    OK_flag = false;
    
    while (!OK_flag && (retries < cmd_retries)) {  //while not OK and bluetooth cmd retries not reached
      
     ERROR_flag = FAIL_flag = false;
     
     if (retries) {  Serial.print("Re-Sending AT Command: "); Serial.println(command); }
      _elm327->print("AT"); if (command[0]) { _elm327->print("+"); _elm327->print(command); } _elm327->print("\r\n"); //sent AT cmd to HC-05
      
     int recvRetry = 0;
     int recv = 0;
     
     do {
      
      _elm327->setTimeout(5000);
       recv += _elm327->readBytesUntil('\n', elm327Buffer + recv , ELM327_BUF_LEN -1 - recv);
       elm327Buffer[recv] = 0; 

       recvRetry++;
       OK_flag = strstr(elm327Buffer, "OK"); if (OK_flag) { elm327Buffer[recv-1] = 0; }
       ERROR_flag = strstr(elm327Buffer, "ERROR"); if (ERROR_flag) { strcpy(elm327Buffer+recv-1, " - ignored."); }
       FAIL_flag = strstr(elm327Buffer, "FAIL"); if (ERROR_flag) { strcpy(elm327Buffer+recv-1, " - ignored."); }
       
     } while (!FAIL_flag && !ERROR_flag && !OK_flag && recvRetry < cmd_retries);    
     
     Serial.println(elm327Buffer);
     delay(1000);
     retries++;   
   }
   
   _isError = ignoreError ? false : !OK_flag;                               //set bluetooth error flag to true
  }
}
