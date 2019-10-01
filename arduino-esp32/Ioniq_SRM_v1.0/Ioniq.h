#ifndef Ioniq_h
#define Ioniq_h
#include <Arduino.h>
#include "ELM327Client.h"


#define IONIQ_VEHICLE_ID "My IONIQ"   // VehicleID is sent in telegram message
#define IONIQ_PHEV_BAT 8.9
#define IONIQ_EV17_BAT 28
#define IONIQ_EV20_BAT 38.6
#define IONIQ_EV17_RANGE_KM 240
#define IONIQ_EV20_RANGE_KM 305
#define IONIQ_OBD_2101 "2101"
#define IONIQ_OBD_2105 "2105"

enum IONIQ_Model {
  IONIQ_PHEV,
  IONIQ_EV17,
  IONIQ_EV20
};

class Ioniq {
private:
  EML327Client *_elm327;
  float _batteryCapacity;
  String _vehicleId;
  IONIQ_Model _model;
  
  bool Ok_2101;
  bool Ok_2105;

  int incr = 0;
  char currentAMP1[3];
  char currentAMP2[3];
  char volt1[3];
  char volt2[3];
  char charger[2];
  char tempMax[3];
  char tempMin[3];
  char tempM1[3];
  char tempM2[3];
  char fan[2];
  char bso[3];
  char cec[9];
  char test2105[25];

  bool executeATCommand(char * atCommand);
  
public:
  Ioniq(EML327Client* elm327);
  Ioniq(EML327Client* elm327, IONIQ_Model model);
  Ioniq(EML327Client* elm327, IONIQ_Model model, char* vehicleId);
  
  enum OBDCommand {
    OBD_2101,
    OBD_2105
  };

  bool isTestMode;
  String  getVehicleId();
  char* getOBDCommandCode(OBDCommand obdCommand);
  bool  getStatus();
  bool  getStatus(OBDCommand obdCommand);
  void  resetStatus();
  bool  isCharging();
  void  resetToCharged();
  bool  update();
  int   executeOBDCommand(char * obdCommand);
  bool  processOBD(OBDCommand obdCommand, char *obdResponse);
  void  printDebugInfo(OBDCommand obdCommand);
  char* extractSubstring(char* dest, char* source, char* pattern, int startPos , int endPos);
  char* testString(OBDCommand obdCommand);
  
  int intFan;
  float bsoDecimal;
  float currentDecimal;
  float currentAMP1Decimal;
  float currentAMP2Decimal;
  float voltageDecimal;
  float powerDecimal;
  float tempDecimal;
  float tempMinDecimal;
  float tempMaxDecimal;
  float cecDecimal;
  float cecInitial = -1;
  float bsoInitial = -1;
  int intCharger;
  int endCharge;
  int intKms;
  int envioInforme = 0;;
};
#endif
