#include "YTGlobals.h"
#include "YTBuzz.h"
#include "YTOTA.h"
#include "YTBatt.h"
#include "YTBTMaster.h"
#include <WiFi.h>
#include <mutex>

int WifiCommandClient;

unsigned long LastEIDTime;
bool oneSendPerTag = false;  //user must release button to send a new tag read
bool oneSentThisTag = false;       //this flag indicates that a tag has been sent this button press
bool TagReadFlag = false;
bool SafeRead = true;


std::mutex CommandsMutex;
String m_WifiCommands[TOTAL_CLIENTS];
String m_WifiResponses[TOTAL_CLIENTS];
String m_WifiAllClientResponses[TOTAL_CLIENTS];
char SerialDataIn[MAX_LINE_LENGTH] = { 0 };



void SetWifiCommand(int ClientNo, const char* Cmd) {
  std::lock_guard<std::mutex> guard(CommandsMutex);
  m_WifiCommands[ClientNo] = String(Cmd);
}


String GetWifiCommand(int ClientNo) {
  std::lock_guard<std::mutex> guard(CommandsMutex);
  return m_WifiCommands[ClientNo];
}


void SetWifiResponse(int ClientNo, const char* Cmd) {
  std::lock_guard<std::mutex> guard(CommandsMutex);
  m_WifiResponses[ClientNo] = String(Cmd);
}


String GetWifiResponse(int ClientNo) {
  std::lock_guard<std::mutex> guard(CommandsMutex);
  return m_WifiResponses[ClientNo];
}


void SetWifiAllClientResponse(int ClientNo, const char* Cmd) {
  std::lock_guard<std::mutex> guard(CommandsMutex);
  m_WifiAllClientResponses[ClientNo] = String(Cmd);
}


String GetWifiAllClientResponse(int ClientNo) {
  std::lock_guard<std::mutex> guard(CommandsMutex);
  return m_WifiAllClientResponses[ClientNo];
}






void CommandsSetup(void) {
#ifdef CMD_DEBUG
  log_fxl("**** COMMAND DEBUG ON ******");
#endif

  Settings.begin("Settings", false);
  oneSendPerTag = Settings.getBool(OneTagReadEEAddr, true);
  SafeRead = Settings.getBool(SafeReadEEAddr, true);
  Settings.end();

  for (int i = 0; i < TOTAL_CLIENTS; i++) {
    SetWifiCommand (i, "");
    SetWifiResponse(i, "");
    SetWifiAllClientResponse(i, "");
  }

  WifiCommandClient = 0;
}




void SendToAllClients(String Data) {
  for (int i = 0; i < TOTAL_CLIENTS; i++) {
    SetWifiAllClientResponse(i, Data.c_str());
  }
}


bool EIDValid(String EID) {
  if (EID.length() > 16) {
    EID.remove(0, EID.length() - 16);
  }

  EID.replace(" ", "");

  if (EID.length() < 15) {
    return false;
  }

  int Count = 0;
  for (int i = EID.length() - 1; i >= 0; i--) {
    if ((EID.charAt(i) < '0') || (EID.charAt(i) > '9')) {
      return false;
    }

    Count = Count + 1;
    if (Count == 15) {
      return true;
    }
  }

  return true;
}



void SendEIDToAllClients(unsigned long Now, String EID) {
  String EID1;
  String EID2;
  String EIDBt;
  bool sendEID = false;

  if (LastEID != EID) {
    oneSentThisTag = false; 
  }

  if (oneSendPerTag && !oneSentThisTag) {
    oneSentThisTag = true;
    sendEID = true;
  } 
  else if ((LastEID != EID) || ((LastEID == EID) && (Now - LastEIDTime > MAX_EID_TIME))) {  //and it's a new tag or we waited a little while
    oneSentThisTag = true;
    sendEID = true;
  }
  else if (!oneSendPerTag) {
    sendEID = true;
  }

  if (sendEID) {
    //log_fxl("EID changed from : %s : %s", LastEID.c_str(), EID.c_str());
    LastEID = EID;
    LastEIDTime = Now;

    if (BtEIDOutputFormat == 1) {
      EIDBt = ZPLCode;  //"^XA^BC,,Y,N,N^FD" + EID + "^FS^XZ";
      EIDBt.replace("EID", EID);
    } else {
      EIDBt = EID;
    }
    EID1 = "[R" + EID + ']';
    EID2 = "{EID" + EID + '}';
    for (int i = 0; i < TOTAL_CLIENTS; i++) {
      if (i == SCALE_CLIENT_IDX) {
        SetWifiAllClientResponse(i, EID2.c_str());
      } else if (i == BT_CLIENT_IDX) {
        SetWifiAllClientResponse(i, EIDBt.c_str());
      } else {
        SetWifiAllClientResponse(i, EID1.c_str());
      }
    }
  }
}


String LastEIDCaptured = "";
bool TagCaptured(volatile byte RawData[], int TagType) {
//   LastTagType = TagType;

//   AvgCount = 0;

//   if (TagType == HDX_TAG_TYPE) {
//     log_fxl("HDX Tag ");
//   } else {
//     log_fxl("SCRAP Tag ");
//   }
//   int ManCode = (RawData[5] << 2) + (RawData[4] >> 6);
//   uint64_t IDLow = (RawData[4] & 0b00111111);
//   IDLow = IDLow << 8;
//   IDLow = IDLow | RawData[3];
//   IDLow = IDLow << 8;
//   IDLow = IDLow | RawData[2];
//   IDLow = IDLow << 8;
//   IDLow = IDLow | RawData[1];
//   IDLow = IDLow << 8;
//   IDLow = IDLow | RawData[0];

//   if ((ManCode >= 100) && (ManCode < 1000)) {
//     TagReadFlag = true;
//     BuzzOn(BUZZ_TAG_READ, 1, 0, false);
//     String TempEID = String(ManCode) + " " + uint64ToString(IDLow);
// #ifdef MAIN_DEBUG
//     log_fxl("%s", TempEID.c_str());
//     if (LastEID != TempEID) {
//       log_fxl("*********** EID has changed from %s ***********************", LastEID.c_str());
//     }
// #endif
//     bool returnResult = false;
//     if (!SafeRead || (TempEID == LastEIDCaptured)) { // safe read means this must be the second time this tag has been read
//       SendEIDToAllClients(millis(), TempEID);
//       returnResult = true;
//     }
//     LastEIDCaptured = TempEID;

// #ifdef MAIN_DEBUG
//     int x = 0;
//     char dataString[5] = { 0 };
//     Serial.print("TAG - ");
//     while (x < HDX_MAX_BUF) {
//       sprintf(dataString, "%02X ", RawData[x]);
//       Serial.print(dataString);
//       //RawData[x] = 0;
//       //Serial.print (" ");
//       x++;
//     }
//     Serial.println();
// #endif

//     return returnResult;
//   }

  return false;
}



String PerformCommand(const char* Cmd, unsigned long Now) {
  String Result = "";
  String Command = Cmd;

  if (Command.startsWith("[WCE")) {//received a weight event
    // Command.remove(0, 4);  //remove the first 3 digital
    // Command.remove(Command.lastIndexOf("]"));
    // WeightEvent TestEvent = static_cast<WeightEvent>(Command.toInt());
    // if (TestEvent != CurrentEvent) {
    //   CurrentEvent = static_cast<WeightEvent>(Command.toInt());
    //   LastEventChange = Now;
    //   log_fxl("Weight change event = %d", CurrentEvent);
    // }
    return String("");
  }

  else if (Command.startsWith("[N]")) {
    // OverrideWeightEvent = Now;
    // return String("[N" + String((unsigned int)NoiseLevel.getLastValue()) + "]");
  }

  else if (Command.startsWith("[V]")) {
    // OverrideWeightEvent = Now;
    // return String("[V" + String((unsigned int)AntVoltsBuf.getLastValue()) + "]");
  }

  else if (Command.startsWith("[VTX")) {
    // OverrideWeightEvent = Now;
    // String Data = "";
    // for (int i=0; i < MAX_TUNE_STEPS; i++) {
    //   Data = Data + String(AntVoltsTx[i]);
    //   if (i < MAX_TUNE_STEPS - 1) {
    //     Data = Data + ",";
    //   }
    // }
    // return String("[VTX" + Data + "]");
  }

  else if (Command.startsWith("[VRX")) {
    // OverrideWeightEvent = Now;
    // String Data = "";
    // for (int i=0; i < MAX_TUNE_STEPS; i++) {
    //   Data = Data + String(AntVoltsRx[i]);
    //   if (i < MAX_TUNE_STEPS - 1) {
    //     Data = Data + ",";
    //   }
    // }
    // return String("[VRX" + Data + "]");
  }

  else if (Command.startsWith("[BC")) {  //send barcode to all clients
    //Serial.println ("Barcode command received.");
    log_fxl("Barcode command received.");
    Command.remove(0, 3);  //remove the "{BC"
    int Index = Command.indexOf("]");
    if (Index > 0) {  //0 will be empty barcode string, so we only want > 0
      String BarCode = Command.substring(0, Index);
      BarCode = "[B" + BarCode + ']';
      SendToAllClients(BarCode);
    }
  }

  else if (strcmp(Cmd, "[ZN]") == 0) {
    Result = String("[YTP001-0]");
    return Result;
  }

  else if (Command.startsWith("{SERIAL}")) {
    Command.remove(0, 8);  //remove the first 8 digital
    SerialNo = Command;
    Settings.begin("Settings", false);
    Settings.putString(SerialNoEEAddr, SerialNo);
    Settings.end();
    Serial.println("Serial number set.");
    return String("{SERIAL_OK}");
  }

  else if (Command.startsWith("{BATTOFF}")) {
    Command.remove(0, 9);  //remove the first 9 digital
    BattVoltOffset = Command.toFloat();
    Settings.begin("Settings", false);
    Settings.putFloat(BattVoltOffsetEEAddr, BattVoltOffset);
    log_fxl("Batt volt offset set to %f", BattVoltOffset);
    BattVoltOffset = Settings.getFloat(BattVoltOffsetEEAddr);
    Settings.end();
    log_fxl("Batt Voltage Offset = %f", BattVoltOffset);
    return String("{BATTOFF_OK}");
  }

  else if (Command.startsWith("{BT_DISCO_START}")) {
//Serial.println ("Serial number set.");
#ifdef LOCAL_BT_IMPLMENTATION
    //SerialBT.begin(BT_NAME, true); //make sure it is in master mode
    ForceBTOn = true;
    SetupBTMaster(false);
    StartBTMasterTask();
    SerialBT.Discover();
#endif
    return String("{BT_DISCO_START_OK}");
  }

  else if (Command.startsWith("{BT_DISCO_DONE}")) {
#ifdef LOCAL_BT_IMPLMENTATION
    if (SerialBT.DiscoverDone()) {
      log_fxl("BT discovery finished");
      return String("{BT_DISCO_FINISHED}");
    } else {
      log_fxl("BT discovery still running");
      return String("{BT_DISCO_RUNNING}");
    }
#endif
  }

  else if (Command.startsWith("{BT_DISCO_RESULT}")) {
#ifdef LOCAL_BT_IMPLMENTATION
    return String("{BT_DISCO_RESULT" + SerialBT.DiscoverResult() + "}");
#endif
  }

  else if (Command.startsWith("{BT_RESET}")) {
    SerialBT.disconnect();
    CurrentWaitTime = MIN_WAIT_TIME;
    LastConnectTime = 0;
    return String("{BT_RESET_OK}");
  }

  else if (Command.startsWith("[F]")) {
    FactoryReset(0);
    return String("[FOK]");
  }

  else if (Command.startsWith("U")) {
    log_fxl("OTA Update command recieved.");
    //this assumes that there variables have been set already. They are set as parameters
    ExecuteOTA(OTAHost, OTAPort, OTAPath);
    OTAUpdating = false;
    log_fxl("Exited OTA Update");
    return String("");
  }

  else if (Command.startsWith("S")) {
    Result = Command;
    Command.remove(0, 1);  //remove the S
    int Param = Command.toInt();
    switch (Param) {
      case 0: Result = Result + String(FIRMWARE_VERSION); break;
      case 1: Result = Result + String(GUIDStr); break;
      case 2: Result = Result + String(ConnectionType); break;
      case 3: Result = Result + String(ProtocolType); break;
      case 4: Result = Result + WiFi.localIP().toString(); break;
      case 5: Result = Result + DefaultSSID; break;
      case 6: Result = Result + DefaultPassword; break;
      case 7: Result = Result + DefaultScaleIP; break;
      case 8: Result = Result + String(BattVolts); break;
      case 9: Result = Result + ssid; break;
      case 10: Result = Result + Password; break;
      case 11: Result = Result + String(BattPercent); break;
      case 12: Result = Result + BTName; break;
      case 13: Result = Result + BTAddress; break;
      case 14: Result = Result + ssid2; break;
      case 15: Result = Result + Password2; break;
      case 16: Result = Result + ssid3; break;
      case 17: Result = Result + Password3; break;
      case 18: Result = Result + String(DataModNo); break;
      case 19: Result = Result + String(WiFi.SSID()); break;
      case 20: Result = Result + String(WiFi.RSSI()); break;
      case 21: Result = Result + String(HARDWARE_VERSION); break;
      case 22: Result = Result + SerialNo; break;
      case 23: Result = Result + BTPin; break;
      case 24: Result = Result + BTDeviceName; break;
      case 25: Result = Result + BTDeviceAddr; break;
      case 26: Result = Result + WifiName; break;
      case 27: Result = Result + String(BattVoltOffset); break;
      case 29: Result = Result + String(QuietMode); break;
      case 36: Result = Result + String(BuzzerFreq); break;
      case 38: Result = Result + String(BtSSPModeEnabled); break;
      case 39: Result = Result + String(BtEIDOutputFormat); break;
      case 41: Result = Result + DefaultStaticIP; break;
      case 42: Result = Result + DefaultStaticGateway; break;
      case 43: Result = Result + DefaultStaticSubnet; break;
      case 44: Result = Result + String(UseStaticIP); break;
    }
    log_fxl("Return: %s", Result.c_str());
    return Result;
  }

  else if (Command.startsWith("P")) {
    Settings.begin("Settings", false);
    Command.remove(0, 1);  //remove the P
    String Param = Command.substring(0, 2);
    if (Param == "00") {                         //connection type
      Command.remove(0, 2);                      //remove the first digit
      int TempConnectionType = Command.toInt();  //we don;t want to change connection type here as it will possibly drop the connection
      Settings.putInt(ConnectionTypeEEAddr, TempConnectionType);
      TempConnectionType = Settings.getInt(ConnectionTypeEEAddr);
      log_fxl("ConnectionType set to %d", TempConnectionType);
    } else if (Param == "01") {  //protocol type
      Command.remove(0, 2);      //remove the first digit
      ProtocolType = Command.toInt();
      Settings.putInt(ProtocolTypeEEAddr, ProtocolType);
    } else if ((Param == "02") || (Param == "04") || (Param == "06") || (Param == "08")) {  //ssid
#ifdef CMD_DEBUG
      log_fxl("SSID command = %s", Command.c_str());
#endif
      Command.remove(0, 2);  //remove the first digit
      int Num = Param.toInt();
      switch (Num) {
        case 2:
          ssid = Command;
          Settings.putString(SSIDEEAddr, ssid);
          break;
        case 4:
          ssid2 = Command;
          Settings.putString(SSID2EEAddr, ssid2);
          break;
        case 6:
          ssid3 = Command;
          Settings.putString(SSID3EEAddr, ssid3);
          break;
        case 8:
          DefaultSSID = Command;
          Settings.putString(DefaultSSIDEEAddr, DefaultSSID);
          break;
      }
    } else if ((Param == "03") || (Param == "05") || (Param == "07") || (Param == "09")) {  //Password
      Command.remove(0, 2);                                                                 //remove the first digit
      int Num = Param.toInt();
      switch (Num) {
        case 3:
          Password = Command;
          Settings.putString(PasswordEEAddr, Password);
          break;
        case 5:
          Password2 = Command;
          Settings.putString(Password2EEAddr, Password2);
          break;
        case 7:
          Password3 = Command;
          Settings.putString(Password3EEAddr, Password3);
          break;
        case 9:
          DefaultPassword = Command;
          Settings.putString(DefaultPasswordEEAddr, DefaultPassword);
          break;
      }
    } else if (Param == "10") {  //set yardsTech scale ip address eg 192.168.4.1
      Command.remove(0, 2);      //remove the first digit
      DefaultScaleIP = Command;
      Settings.putString(DefaultYTIPEEAddr, DefaultScaleIP);
    }

    else if (Param == "11") {  //BT Pin code
      Command.remove(0, 2);    //remove the first 2 digit2
      BTPin = Command;
      Settings.putString(BTPinEEAddr, BTPin);
    }

    else if (Param == "12") {  //BT Device name
      Command.remove(0, 2);    //remove the first 2 digits
      BTDeviceName = Command;
      Settings.putString(BTDeviceNameEEAddr, BTDeviceName);
    }

    else if (Param == "13") {  //BT Device address
      Command.remove(0, 2);    //remove the first 2 digits
      BTDeviceAddr = Command;
      Settings.putString(BTDeviceAddrEEAddr, BTDeviceAddr);
    }

    else if (Param == "14") {  //Local Wifi name
      Command.remove(0, 2);    //remove the first 2 digits
      WifiName = Command;
      Settings.putString(WifiNameEEAddr, WifiName);
    }

    else if (Param == "15") {  //Local BT name
      Command.remove(0, 2);    //remove the first 2 digits
      BTName = Command;
      Settings.putString(BTNameEEAddr, BTName);
    }

    else if (Param == "17") {  //QuietMode
      Command.remove(0, 2);    //remove the first digit
      QuietMode = Command.toInt();
      Settings.putBool(QuietEEAddr, QuietMode);
    }

    else if (Param == "24") {  //BuzzerFreq
      Command.remove(0, 2);    //remove the first digit
      BuzzerFreq = Command.toInt();
      Settings.putInt(BuzzFreqEEAddr, BuzzerFreq);
    }

    else if (Param == "26") {  //Bt SSP Mode enabled
      Command.remove(0, 2);    //remove the first digit
      BtSSPModeEnabled = Command.toInt();
      Settings.putBool(BtSSPModeEnabledEEAddr, BtSSPModeEnabled);
    }

    else if (Param == "27") {  //Bt outoput format for eid 0=plain text, 1=ZPL barcode
      Command.remove(0, 2);    //remove the first digit
      BtEIDOutputFormat = Command.toInt();
      Settings.putInt(BtEIDOutputFormatEEAddr, BtEIDOutputFormat);
    }

    else if (Param == "29") {  //set static ip address eg 192.168.4.20
      Command.remove(0, 2);    //remove the first digit
      DefaultStaticIP = Command;
      Settings.putString(DefaultStaticIPEEAddr, DefaultStaticIP);
    }

    else if (Param == "30") {  //set static gateway ip address eg 192.168.4.1
      Command.remove(0, 2);    //remove the first digit
      DefaultStaticGateway = Command;
      Settings.putString(DefaultStaticGatewayEEAddr, DefaultStaticGateway);
    }

    else if (Param == "31") {  //set OTA Host address eg www.farmxl.com.au
      Command.remove(0, 2);    //remove the first digit
      OTAHost = Command;
    }

    else if (Param == "32") {  //set OTA path eg /firmare_updates/v20200122.ino.bin
      Command.remove(0, 2);    //remove the first digit
      OTAPath = Command;
    }

    else if (Param == "33") {  //OTA Port number
      Command.remove(0, 2);    //remove the first digit
      OTAPort = Command.toInt();
    }

    else if (Param == "34") {  //set static subnet ip address eg 255.255.255.0
      Command.remove(0, 2);    //remove the first digit
      DefaultStaticSubnet = Command;
      Settings.putString(DefaultStaticSubnetEEAddr, DefaultStaticSubnet);
    }

    else if (Param == "35") {  //use thestatic IP
      Command.remove(0, 2);    //remove the first digit
      UseStaticIP = Command.toInt();
      Settings.putBool(UseStaticIPEEAddr, UseStaticIP);
    }

    Settings.end();

    Result = String("P" + Param + "OK");
    return Result;
  }

  else {
    return Result;
  }

  return Result;
}


void CommandsTask(void * parameter) {  
//void CommandsLoop() {
  while (!OTAUpdating && !ExternalPoweredOn && !PoweringDown) {
    vTaskDelay(1);//pdMS_TO_TICKS(1000));
    unsigned long Now = millis();
    if (WifiCommandClient >= TOTAL_CLIENTS) {
      WifiCommandClient = 0;
    }

    if (GetWifiCommand (WifiCommandClient).length() > 0) {
      SetWifiResponse(WifiCommandClient, PerformCommand(GetWifiCommand (WifiCommandClient).c_str(), Now).c_str());
      SetWifiCommand (WifiCommandClient, "");
    }

    WifiCommandClient = WifiCommandClient + 1;

    // if (Serial2.available() > 0) {
    //   char c = Serial2.read();
    //   char buffer[3];     // Buffer size must be at least 3 (2 digits + null terminator)

    //   sprintf(buffer, "%02X ", c);
    //   Serial.print(buffer); 
    // }

    // If there is some serial command come in...
    if (Serial.available() > 0) {
      // Read the data
      char newChar = Serial.read();
  #ifdef CMD_DEBUG
      log_fxl("Command char %c", newChar);
  #endif

      // If we have the end of a string
      // (Using the test your code uses)
      if (newChar == '\r') {
  #ifdef CMD_DEBUG
        log_fxl("Recieved data : %s", SerialDataIn);
  #endif
        Serial.write(PerformCommand(SerialDataIn, Now).c_str());
        // Empty the string for next time
        SerialDataIn[0] = 0;
      }

      else if (newChar == '\n') {
        //discard the LF
      }

      else if ((newChar >= ' ') && (newChar <= '~')) {  //must be valid char between space and ~
        // Add it to the string
        char S[2];
        S[0] = newChar;
        S[1] = '\0';
        if (strlen(SerialDataIn) < MAX_LINE_LENGTH - 2) {
          strcat(SerialDataIn, S);
        } else {
          SerialDataIn[0] = 0;
        }
      }
    }
  }
  log_fxl("Commands Task stopped!");
  vTaskDelete (NULL);
}


void StartCommandsTask (){
  xTaskCreate(
                  CommandsTask,          /* Task function. */
                  "CommandsTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  HIGHEST_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}
