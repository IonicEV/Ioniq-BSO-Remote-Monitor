#include "Ioniq.h"

static int getBatteryCapacity(IONIQ_Model model) {
    switch(model) {
      case IONIQ_PHEV:
        return IONIQ_PHEV_BAT;
      case IONIQ_EV17:
        return IONIQ_EV17_BAT;
      case IONIQ_EV20:
        return IONIQ_EV20_BAT;
      default:
        return IONIQ_PHEV_BAT;
    }
}

Ioniq::Ioniq(EML327Client* elm327) {
  _elm327 = elm327;
  _model = IONIQ_PHEV;
  _batteryCapacity = getBatteryCapacity(_model);
  _vehicleId = IONIQ_VEHICLE_ID;
}

Ioniq::Ioniq(EML327Client* elm327, IONIQ_Model model) {
  _elm327 = elm327;
  _model = model;
  _batteryCapacity = getBatteryCapacity(_model);
  _vehicleId = IONIQ_VEHICLE_ID;
}

Ioniq::Ioniq(EML327Client* elm327, IONIQ_Model model, char* vehicleId) {
  _elm327 = elm327;
  _model = model;
  _batteryCapacity = getBatteryCapacity(_model);
  _vehicleId = vehicleId;
}

char* Ioniq::getOBDCommandCode(OBDCommand obdCommand) {
  switch (obdCommand) {
    case OBD_2101: return IONIQ_OBD_2101;
    case OBD_2105: return IONIQ_OBD_2105;
    default: return NULL;
  }
}

String Ioniq::getVehicleId() {
  return _vehicleId;
}

bool Ioniq::getStatus() {
  return Ok_2101 && Ok_2105;
}

bool Ioniq::getStatus(OBDCommand obdCommand) {
   switch (obdCommand) {
    case OBD_2101: return Ok_2101;
    case OBD_2105: return Ok_2105;
  } 
}

void Ioniq::resetStatus() {
  Serial.println ("OBD Status reset");
  Ok_2101 = 0;
  Ok_2105 = 0;
}

bool Ioniq::isCharging() {
  return intCharger && powerDecimal < 0;
}

void Ioniq::resetToCharged() {
  cecInitial = cecDecimal;
  bsoInitial = bsoDecimal;
  envioInforme = 0;
}

bool Ioniq::processOBD(OBDCommand obdCommand, char *obdResponse) {
  Serial.print("Processing OBD: "); Serial.print(getOBDCommandCode(obdCommand));
  if (OBD_2101 == obdCommand) {
    Ok_2101 = false;
    if (obdResponse) {
      extractSubstring(currentAMP1, obdResponse, "1:", 21 , 23);
      extractSubstring(currentAMP2, obdResponse, "2:", 3 , 5);
      extractSubstring(volt1, obdResponse, "2:", 6 , 8);
      extractSubstring(volt2, obdResponse, "2:", 9 , 11);
      extractSubstring(charger, obdResponse, "1:", 18 , 19);
      extractSubstring(tempMax, obdResponse, "2:", 12 , 14);
      extractSubstring(tempMin, obdResponse, "2:", 15 , 17);
      extractSubstring(tempM1, obdResponse, "2:", 18 , 20);
      extractSubstring(tempM2, obdResponse, "2:", 21 , 23);
      extractSubstring(fan, obdResponse, "4:", 10 , 11);
  
      //CEC
      char p1[3],p2[3],p3[3],p4[3];
      snprintf(cec, 9,  "%s%s%s%s",
        extractSubstring(p1, obdResponse, "5:", 21 , 23),
        extractSubstring(p2, obdResponse, "6:", 3 , 5),
        extractSubstring(p3, obdResponse, "6:", 6 , 8),
        extractSubstring(p4, obdResponse, "6:", 9 , 11)
      );
  
      currentAMP1Decimal = strtol(currentAMP1, NULL, 16);
      currentAMP2Decimal = strtol(currentAMP2, NULL, 16);
      voltageDecimal = (strtol(volt1, NULL,16) * 256 + strtol(volt2, NULL, 16)) / 10.00;
      tempDecimal = (strtol(tempM1, NULL, 16) + strtol(tempM2, NULL,16)) / 2.00;
      tempMaxDecimal = strtol(tempMax, NULL, 16);
      tempMinDecimal = strtol(tempMin, NULL, 16);
      cecDecimal = strtol(cec, NULL, 16) / 10.00;
      intFan = strtol(fan, NULL, 16);
      currentDecimal = 0;
      intCharger = 0;
  
      if (volt1[0] && volt2[0] && currentAMP1[0] && currentAMP2[0] && charger[0] && tempM1[0] && tempM2[0] && tempMin[0] && tempMax[0] && fan[0]) {
        Ok_2101 = true;
        // consume or charge battery? Surely there are better ways to do it. Sorry.
        if (currentAMP1Decimal > 127) {
          currentDecimal = (((currentAMP1Decimal - 256) * 256) + currentAMP2Decimal) / 10.00;
        } else {
          currentDecimal = ((currentAMP1Decimal * 256) + currentAMP2Decimal) / 10.00;
        }
  
        intCharger = strtol(charger, NULL, 16);
        if (intCharger >= 7) {
          intCharger = 1;
        } else {
          intCharger = 0;
        }
        if (intCharger == 1 & powerDecimal < 0) { //Estimation of charge time
          endCharge = ((((100 - bsoDecimal) * _batteryCapacity) / 100) / ((powerDecimal * -1) / 60));
          envioInforme = 1;
        } else {
          endCharge = 0;
        }
  
        if(cecInitial < 0) {
          cecInitial = cecDecimal;
        }
        return 1;
      }
    }
  } else if (OBD_2105 == obdCommand) {
    Ok_2105 = false;
    if (obdResponse) {
      if(IONIQ_PHEV == _model) {   
        extractSubstring(bso, obdResponse, "5:", 21 , 23);
      } else if(IONIQ_EV17 == _model || IONIQ_EV20 == _model) {   
        extractSubstring(bso, obdResponse, "4:", 21 , 23);
      }
      bsoDecimal = strtol(bso, NULL, 16) / 2.00;
  
      if (bso[0]) {
        Ok_2105 = true;
        
        if(bsoInitial < 0) {
          bsoInitial = bsoDecimal;
        }
  
        if (bsoDecimal == 99.50) { //Small trap due to the shutdown of the ECU. Upon arrival the charge is 100%, the ECU goes off. Sorry. Ideas are accepted ;-)
          bsoDecimal = 100;
          currentDecimal = 0;
          voltageDecimal = 397.80;
          intCharger = 0;
        }
    
    
        if (bsoDecimal >= 20) {     //Rule of three calculator for mileage estimate in electrical
          intKms = (bsoDecimal - 20) * 0.7875;
        } else {
          intKms = 0;
        }
        powerDecimal = (voltageDecimal * currentDecimal) / 1000; //Calculate power in kW
        return 1;
      }
    }    
  }
  return false; // unsuccessfull
}


void Ioniq::printDebugInfo(OBDCommand obdCommand) {
  if (OBD_2101 == obdCommand) {
    Serial.println();
    Serial.print ("2101 OK: ");Serial.println(Ok_2101);
    Serial.print("Time charge: ");
    Serial.println(endCharge);

    Serial.print ("CURRENT HEX: ");
    Serial.print(currentAMP1); Serial.print("  ") + Serial.println(currentAMP2);
    Serial.print ("CURRENT AMP: ");
    Serial.print (currentAMP1Decimal, 2);
    Serial.print (" ");
    Serial.println (currentAMP2Decimal, 2);
    Serial.print ("CURRENT DEC: ");
    Serial.println (currentDecimal, 2);
    Serial.print ("VOLTAGE HEX: ");
    Serial.print(volt1); Serial.print(" "); Serial.println(volt2);
    Serial.print ("VOLTAGE DEC: ");
    Serial.println (voltageDecimal, 2);
    Serial.print ("CHARGER HEX: ");
    Serial.println (charger);
    Serial.print ("CHARGER DEC: ");
    Serial.println (intCharger);
    Serial.print ("CEC HEX: ");
    Serial.println (cec);
    Serial.print ("CEC DEC: ");
    Serial.println (cecDecimal);
    Serial.print ("CEC Initial: ");
    Serial.println (cecInitial);

    Serial.print ("TEMP M1 HEX: ");
    Serial.println (tempM1);
    Serial.print ("TEMP M2 HEX: ");
    Serial.println (tempM2);
    Serial.print ("TEMP DEC: ");
    Serial.println (tempDecimal);
    Serial.print ("TEMP Min HEX: ");
    Serial.println (tempMin);
    Serial.print ("TEMP Min DEC: ");
    Serial.println (tempMinDecimal);
    Serial.print ("TEMP Max HEX: ");
    Serial.println (tempMax);
    Serial.print ("TEMP Max DEC: ");
    Serial.println (tempMaxDecimal);
    Serial.print ("FAN: ");
    Serial.println (intFan);
  } else if (OBD_2105 == obdCommand) {
    Serial.println ();
    Serial.print ("2105 OK: ");Serial.println(Ok_2105);
    Serial.print ("BSO HEX: ");
    Serial.println (bso);
    Serial.print ("BSO DEC: ");
    Serial.println (bsoDecimal);
    Serial.println ("");
  }
}
// ***************************************************************************
// Function for extracting data from OBD responses
// ***************************************************************************
char* Ioniq::extractSubstring(char* dest, char* source, char* pattern, int startPos , int endPos) {
    char* pos;
    dest[0]=0;
    pos = strstr(source, pattern);
    if (pos != NULL) {
      strncpy(dest, pos + startPos, endPos-startPos);
      dest[endPos-startPos] = 0;
    } 
    return dest;
}

// Execure OBD command: Send, receive and parsing OBD response
//
bool Ioniq::update() {
  int recv;
  resetStatus();
    if(_elm327 == NULL) {
    Serial.println("Stream is null pointer");
    return getStatus();
  }
  recv = _elm327->executeOBDCommand(getOBDCommandCode(Ioniq::OBD_2101));
  // ***************************************************************************
  // Parsing 2101 command OBD (Volts, intensity, temperatures, etc)
  // ***************************************************************************
  if (isTestMode) { //For testing without OBD connect
    strncpy(_elm327->elm327Buffer, testString(OBD_2101), ELM327_BUF_LEN); _elm327->elm327Buffer[ELM327_BUF_LEN-1] = 0;
  }

  processOBD(OBD_2101, _elm327->elm327Buffer);
  printDebugInfo(OBD_2101);

  recv = _elm327->executeOBDCommand(getOBDCommandCode(Ioniq::OBD_2105));
  // ***************************************************************************
  // Parsing 2105 command OBD (BSO Display)
  // ***************************************************************************
  if (isTestMode) { //For testing without OBD connect
    strncpy(_elm327->elm327Buffer, testString(OBD_2105), ELM327_BUF_LEN); _elm327->elm327Buffer[ELM327_BUF_LEN-1] = 0;
  }
  processOBD(OBD_2105, _elm327->elm327Buffer);
  printDebugInfo(Ioniq::OBD_2105);
  
  return getStatus();
}

// Go ahead and try other values XD
// ***************************************************************************
//
char* Ioniq::testString(OBDCommand obdCommand) {
  if (incr >= 50) {
    //incr=0;
    return "";
  }
  //obdSerial= "1: 19 12 5C 16 F8 70 01\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 01 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 A3 00 00 2F E0 "; // General
  if (OBD_2101 == obdCommand) {
  
    incr++;
    Serial.print("INCREMENTAL: ");
    Serial.println(incr);
  
    if (incr > 0 && incr < 10) {
      return "1: 19 12 5C 16 F8 70 01\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 A3 00 00 2F E0 "; // Con cargador sin cargar 1398,7
    }
  
    if (incr >= 10 && incr < 15) {
      return "1: 19 12 5C 16 F8 70 FE\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 A3 00 00 2F E0 "; // Con cargador y cargando  Lectura: 13987.00
    }
  
    if (incr >= 15 && incr < 20) {
      return "1: 19 12 5C 16 F8 70 FE\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 01 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 AF 00 00 2F E0 "; // Con cargador, ventilador y cargando 1399,9
    }
  
    if (incr >= 20 && incr < 25) {
      return "1: 19 12 5C 16 F8 70 00\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 B2 00 00 2F E0 "; // Con cargador y SIN CARGAR 1400,2
    }
  
    // Repeat other test
  
    if (incr >= 25 && incr < 30) {
      return "1: 19 12 5C 16 F8 00 01\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 C5 00 00 2F E0 "; // SIN CARGADOR 1402,1
    }
  
    if (incr >= 30 & incr < 35) {
      return "1: 19 12 5C 16 F8 70 01\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 C5 00 00 2F E0 "; // Con cargador sin cargar 1402,1
    }
  
    if (incr >= 35 && incr < 40) {
      return "1: 19 12 5C 16 F8 70 FE\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 C5 00 00 2F E0 "; // Con cargador y cargando  Lectura: 1402,1
    }
  
    if (incr >= 40 && incr < 45) {
      return "1: 19 12 5C 16 F8 70 FE\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 01 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 CF 00 00 2F E0 "; // Con cargador, ventilador y cargando 1403,1
    }
  
    if (incr >= 45 && incr < 50) {
      return "1: 19 12 5C 16 F8 00 00\r\n2: 3F 0F 39 15 16 17 18\r\n4: B7 29 00 FF 90 00 00\n\r5: 89 BE 00 00 89 3A 00\n\r6: 00 36 CF 00 00 2F E0 "; // SIN cargador 1403,1
    }
  } else if (obdCommand == OBD_2105) {
  if(IONIQ_PHEV == _model) {   
    snprintf(test2105, 25, "%s%02d", "5: 00 06 02 03 E8 51 ", incr + 25);
  } else if(IONIQ_EV17 == _model || IONIQ_EV20 == _model) {   
    snprintf(test2105, 25, "%s%02d", "4: 00 06 02 03 E8 51 ", incr + 25);
  }
  return test2105;
  }
  return "";
}
