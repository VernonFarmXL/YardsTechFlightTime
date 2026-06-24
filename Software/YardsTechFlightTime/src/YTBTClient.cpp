#include "BluetoothSerial.h" //https://github.com/espressif/arduino-esp32/tree/master/libraries/BluetoothSerial
#include "esp_bt_device.h"
#include "sdkconfig.h"
#include "YTGlobals.h"
#include "YTBuzz.h"
#include "YTWifi.h"
#include "YTCommands.h"


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


void SetupBTClient() {
  #ifdef BT_DEBUG
  log_fxl ("**** BT DEBUG ON - CLIENT ******");
  #endif
  Settings.begin("Settings", false);  
  //all settings are initialised in the BTMaster setup, so it must run first
  Settings.end();

  if (!SafeMode) {
    BTClientConnected = false;
    LastBTClientConnected = false;
    
    #ifdef BT_DEBUG 
      log_fxl ("In Client. Connection Type = %d", CurrentConnectionType); 
    #endif
    if (CurrentConnectionType == CONN_BT_CLIENT) {
      if (BtSSPModeEnabled) {
        SerialBT.enableSSP();
      }
      
      #ifdef BT_DEBUG 
        log_fxl("Setting name = %s", BTName.c_str()); 
      #endif
      
      SerialBT.begin(BTName, false); //Bluetooth device name
      
      printDeviceAddress ();
      //Serial.println("The device started, now you can pair it with bluetooth!");
    }
  
    BtInput[0] = 0;
  }
}


void CloseBTClient() {
  if (CurrentConnectionType == CONN_BT_CLIENT) {
    SerialBT.end(); 
  } 
}

//void BTClientLoop(void * parameter) {  
void BTClientLoop() {
  if (! BtDisabled && (CurrentConnectionType == CONN_BT_CLIENT)) {
    #ifdef BT_DEBUG 
      //Serial.print("Has client = "); 
      //Serial.println(SerialBT.hasClient()); 
    #endif
    LastBTClientConnected = BTClientConnected;
    BTClientConnected = SerialBT.hasClient();
    if (BTClientConnected != LastBTClientConnected) {
      #ifdef BT_DEBUG 
        log_fxl ("BT Client connected changed = %d", BTClientConnected); 
      #endif
      if (BTClientConnected) {
        BuzzOn (BUZZ_BT_CONNECT, 1, QUIET_BT_CONNECT, true);
        CommsPin = HIGH;
        digitalWrite(COMMS_PIN, CommsPin);     
      }
      else {
        BuzzOn (BUZZ_BT_CONNECT, 3, QUIET_BT_CONNECT, true);
      }
    }
  
    //connected to pc as a bluetooth device
    if (CurrentConnectionType == CONN_BT_CLIENT) {
      if (GetWifiResponse(BT_CLIENT_IDX).length() > 0){ //check if there is a command to send
        #ifdef BT_DEBUG
          log_fxl ("Sending data 1 : %s", GetWifiResponse(BT_CLIENT_IDX).c_str());
        #endif
        //SerialBT.write (WifiResponses[BT_CLIENT_IDX].c_str()[0]);//, WifiResponses[BT_CLIENT_IDX].length()));
        SerialBT.println (GetWifiResponse(BT_CLIENT_IDX).c_str());
        SetWifiResponse(BT_CLIENT_IDX, "");
      }
      
      else if (GetWifiAllClientResponse(BT_CLIENT_IDX).length() > 0){ //nothing in the queue, check if there is a command to send
        #ifdef BT_DEBUG
          log_fxl ("Sending data 3 : %s", GetWifiAllClientResponse(BT_CLIENT_IDX).c_str());
        #endif
        SerialBT.println (GetWifiAllClientResponse(BT_CLIENT_IDX).c_str());
        SetWifiAllClientResponse(BT_CLIENT_IDX, "");
      }
      
      else if (SerialBT.available()) {
        char C = SerialBT.read();

        #ifdef BT_DEBUG 
          log_fxl ("%c", C); 
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
            log_fxl ("Recieved data : %s", BtInput); 
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
  
//  log_fxl("BTClientTask stopped!");
//  vTaskDelete (NULL);

}
