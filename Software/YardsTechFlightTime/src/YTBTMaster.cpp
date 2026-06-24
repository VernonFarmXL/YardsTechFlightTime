//Test for master mode:
//1. Connect to bt V2.1 device.
//2. Connect to blue solie.
//3. Connect to windows bluetooth.
//4. Does not crach or halt when left unconnected.
//5. Reconnects if unit is disconnected.
//5. Stays connected to reader over long period.


#include "esp_bt_device.h"
#include "sdkconfig.h"
#include "YTGlobals.h"
#include "YTOTA.h"
#include "YTCommands.h"


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


//uint8_t TestAddress[6]  = {0x00, 0x01, 0x95, 0x2D, 0x98, 0x05};
uint8_t TestAddress[6]  = {0x00, 0x04, 0x3E, 0x94, 0xC7, 0x9B};



void SetupBTMaster(bool Reconnect) {
  #ifdef BT_DEBUG
  log_fxl ("**** BT DEBUG ON - MASTER ******");
  #endif
  Settings.begin("Settings", false);  
  BTName = Settings.getString(BTNameEEAddr);
  BTDeviceName = Settings.getString(BTDeviceNameEEAddr, "");
  BTDeviceAddr = Settings.getString(BTDeviceAddrEEAddr, "");
  BTPin = Settings.getString(BTPinEEAddr, "");
  BtSSPModeEnabled = Settings.getBool(BtSSPModeEnabledEEAddr, false);
  BtEIDOutputFormat = Settings.getInt(BtEIDOutputFormatEEAddr, 0); //default to plain text
  ZPLCode = Settings.getString(ZPLCodeEEAddr, "");
  Settings.end();

  if (!SafeMode) {
    if ((CurrentConnectionType == CONN_BT_MASTER) || ForceBTOn) {
//      SerialBT.setPin(BTPin.c_str());
//      if (BTPin.length() == 0){
      if (BtSSPModeEnabled) {
        SerialBT.enableSSP();
      }
      SerialBT.begin(BTName, true); //Bluetooth device name
      printDeviceAddress ();
    }
  
    if (!Reconnect) {
      LastConnectTime = millis();
      CurrentWaitTime = MIN_WAIT_TIME; //set to 2 seconds, just to give everything a change to turn on
    }
    BtInput[0] = 0;
  }
}


//void BTMasterLoop(unsigned long Now) {
void BTMasterLoop(void * parameter) {  
  log_fxl("BTMasterTask Started");
  while (!OTAUpdating && !ExternalPoweredOn && !PoweringDown && !SafeMode) {
    delay (BTMAST_LOOP_DELAY);
    unsigned long Now = millis();

    if ((!BtDisabled && (CurrentConnectionType == CONN_BT_MASTER))  || ForceBTOn) {
      #ifdef LOCAL_BT_IMPLMENTATION
        if (!SerialBT.hasClient() && SerialBT.ConnectDone()){//currently have no client connected and not attempting a connection, so lets see if we can connect to it
      #else
        if (!SerialBT.hasClient()){//currently have no client connected and not attempting a connection, so lets see if we can connect to it
      #endif
        #ifdef LOCAL_BT_IMPLMENTATION
          if (!SerialBT.DiscoverDone()) { //this stops the bt from reconnecting as soon as discovery is finished
            LastConnectTime = Now;
          }
          
          if ((SerialBT.DiscoverDone()) && (Now - LastConnectTime > CurrentWaitTime)){
        #else
          if ((Now - LastConnectTime > CurrentWaitTime)){
        #endif
          log_fxl("Reconnecting to BT slave device. Wait time = %d, Time diff = %d", CurrentWaitTime, Now - LastConnectTime);
          CurrentWaitTime = CurrentWaitTime + WAIT_TIME_INC; //slowly increase the wait time until we reach the max wait time;
          if (CurrentWaitTime > MAX_WAIT_TIME){
            CurrentWaitTime = MAX_WAIT_TIME;
          }
          LastConnectTime = Now;
          if ((BTDeviceName.length() > 0) || (BTDeviceAddr.length() >= 17)){//one or the other must be set otherwise we are not using bt
            #ifdef BT_DEBUG 
              Serial.println ("Looking to connect to name/address :"); 
              Serial.println (BTDeviceName); 
              Serial.println (BTDeviceAddr); 
            #endif
                        
            log_fxl("Looking to connect to name/address : %s / %s", BTDeviceName.c_str(), BTDeviceAddr.c_str());
                     
            if (BTDeviceAddr.length() >= 17){
              esp_bd_addr_t Addr;
              Addr[0] = strtoul(String(BTDeviceAddr).substring(0, 2).c_str(), NULL, 16);
              Addr[1] = strtoul(String(BTDeviceAddr).substring(3, 5).c_str(), NULL, 16);
              Addr[2] = strtoul(String(BTDeviceAddr).substring(6, 8).c_str(), NULL, 16);
              Addr[3] = strtoul(String(BTDeviceAddr).substring(9, 11).c_str(), NULL, 16);
              Addr[4] = strtoul(String(BTDeviceAddr).substring(12, 14).c_str(), NULL, 16);
              Addr[5] = strtoul(String(BTDeviceAddr).substring(15, 17).c_str(), NULL, 16);
              #ifdef BT_DEBUG 
                Serial.println ("Connecting BT via address"); 
              #endif          
              SerialBT.connect(Addr);
            }
            else {
              #ifdef BT_DEBUG 
                Serial.println ("Connecting BT via name"); 
              #endif          
              SerialBT.connect(BTDeviceName);
            }
          }
        }
  
        //return;
      }
  
      //bail out if no connection
      if (SerialBT.hasClient()){
        CurrentWaitTime = MIN_WAIT_TIME; //this way if we disconnect, we start at the min wait time to reconnect
        
        if (BTDeviceAddr.length() < 17){
          #ifdef LOCAL_BT_IMPLMENTATION
            BTDeviceAddr = SerialBT.ClientAddr();
            Settings.begin("Settings", false);
            Settings.putString(BTDeviceAddrEEAddr, BTDeviceAddr);
            Settings.end();
          #endif
        }
        
         //connected to pc as a bluetooth device
        if ((CurrentConnectionType == CONN_BT_MASTER) || ForceBTOn) {
          if (GetWifiResponse(BT_CLIENT_IDX).length() > 0){ //check if there is a command to send
            #ifdef BT_DEBUG
              Serial.print ("Sending data : ");
              Serial.println (GetWifiResponse(BT_CLIENT_IDX).c_str());
            #endif
            SerialBT.println (GetWifiResponse(BT_CLIENT_IDX).c_str());
            SetWifiResponse(BT_CLIENT_IDX, "");
          }
                   
          else if (GetWifiAllClientResponse(BT_CLIENT_IDX).length() > 0){ //nothing in the queue, check if there is a command to send
            #ifdef BT_DEBUG
              Serial.print ("Sending data : ");
              Serial.println (GetWifiAllClientResponse(BT_CLIENT_IDX).c_str());
            #endif
            SerialBT.println (GetWifiAllClientResponse(BT_CLIENT_IDX).c_str());
            SetWifiAllClientResponse(BT_CLIENT_IDX,"");
          }
          
          else if (SerialBT.available()) {
            char C = SerialBT.read();
    
            #ifdef BT_DEBUG 
              Serial.println(C); 
            #endif
      
            if ((C >= ' ') && (C <= '~')) { //must be valid char between space and ~
              // Add it to the string
              char S[2];
              S[0] = C;
              S[1] = '\0';
              if (strlen(BtInput) < MAX_BUFFER_LENGTH - 2){
                strcat(BtInput, S);
              }
              else {
                BtInput[0] = 0;
              }
    
              if ((C == '?') || (C == '}')) { //this is needed for the ruddweigh or tru test protocol as it has no CR
                C = '\r';
              }
            }
            
            // If we have the end of a string
            if (C == '\r') {
              #ifdef BT_DEBUG 
                Serial.println ("Recieved data :"); 
                Serial.println (BtInput); 
              #endif
              SetWifiCommand (BT_CLIENT_IDX, BtInput);
      
              // Empty the string for next time
              BtInput[0] = 0;
            }
            
            else if (C == '\n') {
              //discard the LF 
            }        
          }
          
        }  

      }    
    }
  
  }

  log_fxl("BTMasterTask stopped!");
  vTaskDelete (NULL);

}


void StartBTMasterTask (){
  if ((CurrentConnectionType == CONN_BT_MASTER) || ForceBTOn) {
    xTaskCreate(
                    BTMasterLoop,          /* Task function. */
                    "BTMasterLoop",        /* String with name of task. */
                    10000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    BTMAST_LOOP_PRIORITY,               /* Priority of the task. */
                    NULL);            /* Task handle. */
  }
}
