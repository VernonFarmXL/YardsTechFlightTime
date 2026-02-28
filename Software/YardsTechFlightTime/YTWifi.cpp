//YTWifi.cpp

#include <WiFi.h>
#include <AsyncUDP.h>
#include "YTGlobals.h"
#include "YTCommands.h"
#include "YTOTA.h"
#include "YTBuzz.h"
#include "YTWifi.h"

#define DEF_PASSWORD "" //don't use a password //"yardstech"
#define PING "[!]"
#define UPD_BROADCAST_ADDR "255.255.255.255"
#define UPD_PORT 15000
#define TCP_PORT 20000 //for general TCP comms

//#define MAX_LINE_LENGTH 50
#define MAX_MISSED_PING_TIME 25000 //max time between missed pings before client is disconnected
#define PING_TIME 5000 //ms between ping
#define WIFI_CONNECT_TIMEOUT 5000
#define SCALE_CONNECT_TIMEOUT 5000
#define YT_WAIT_TIMEOUT 30000
#define SCALE_WEIGHT_TIMEOUT 30000 //send WCE command every 30 seconds

//#include <AsyncUDP.h>
//#include "YTGlobals.h"

//const char* ssid = "YardsTech";
//const char* password = "yardstech";
//const char* mqtt_server = "192.168.10.1";

//const char* ssid = "mavn-wireless4";
//const char* password = "qwertyuiopasd";
//const char* mqtt_server = "192.168.10.23";


WiFiServer server(TCP_PORT);
WiFiClient *clients[MAX_CLIENTS + 1] = { NULL }; //the +1 is where the YT client is stored
WiFiClient ScaleClient;
char inputs[MAX_CLIENTS + 1][MAX_LINE_LENGTH] = { 0 };
unsigned long MissedPings[MAX_CLIENTS + 1];
WiFiUDP udp;
unsigned long LastWifiConnectAttempt = 0;
unsigned long LastScaleConnectAttempt = 0;
unsigned long LastYTWaitTime = 0;
unsigned long LastWifiConnectAttemptLED = 0;
unsigned long LastPing = 0;
unsigned long LastScaleWeight = 0;
bool LastScaleConnectStatus = false;
bool YTWaitTimeExceeded = false;
int LastWifiStatus = WL_IDLE_STATUS;
int CommsPin = HIGH;
//int ClientNo = 0;
IPAddress DefSubnet(255,255,255,0);
bool AwaitingWeightResponse = false;
bool RestartingWifi = false;



void ScanForScales() {
  char packetBuffer[255]; //buffer to hold incoming packet
  // if there's data available, read a packet
  int packetSize = udp.parsePacket();
  if (packetSize) {
    IPAddress remoteIp = udp.remoteIP();
    log_i("Received packet of size %d from %s, port %d", packetSize, remoteIp.toString().c_str(), udp.remotePort());

    // read the packet into packetBufffer
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    String buf = packetBuffer;
    log_i ("Contents: %s", buf.c_str());

    if (buf.startsWith("FXL-YTS")) {
      DefaultScaleIP = udp.remoteIP().toString();
      log_i ("Setting Scale IP to: %s", DefaultScaleIP.c_str());      
    }
  }
}



void ConnectToScaleTask(void * parameter) {  
  // log_i("ConnectToScaleTask Started");
  // while (!OTAUpdating && !PoweringDown) {
  //   vTaskDelay(pdMS_TO_TICKS(1000));
  //   unsigned long Now = millis();
  //   if (!ScaleClient.connected()) {
  //     CurrentEvent = weDisabled;
  //     ScanForScales(); //see if we can find the scales using UPD

  //     if (Now - LastScaleConnectAttempt > SCALE_CONNECT_TIMEOUT) {
  //       #ifdef WIFI_DEBUG
  //         log_i ("Trying to connect to scale.");
  //       #endif

  //       LastScaleConnectAttempt = Now;
  //       ScaleClient.connect(DefaultScaleIP.c_str(), TCP_PORT, 1000);
  //     }
  //   }
  //   //else if (!AwaitingWeightResponse && (Now - LastScaleWeight > SCALE_WEIGHT_TIMEOUT)) {
  //   else if (Now - LastScaleWeight > SCALE_WEIGHT_TIMEOUT) {
  //     SetWifiResponse(SCALE_CLIENT_IDX, "[WCE1]"); 
  //     LastScaleWeight = Now;
  //     //AwaitingWeightResponse = true;
  //   }
  // }

  log_i("ConnectToScaleTask stopped!");
  vTaskDelete (NULL);
}


void StartConnectToScaleTask (){
  xTaskCreate(
                  ConnectToScaleTask,          /* Task function. */
                  "ConnectToScaleTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  NORMAL_PRI,               /* Priority of the task. */
                  NULL);            /* Task handle. */
}


void ConnectToWifiNetwork() {
  String TempSSID = "";

  if ((CurrentConnectionType == CONN_WAIT_YT_THEN_AP) || (CurrentConnectionType == CONN_YT)) {
    String tempS = DefaultSSID;
    TempSSID = tempS;
    log_i ("TempSSID = %s, tempS = %s", TempSSID.c_str(), tempS.c_str());
    WifiNum = 4; //set number above the normal range to signify YardsTech network
  }
  else {
    WifiNum = WifiNum + 1;
    if (WifiNum > 3) {
      WifiNum = 0;
      return; //we have hit the end of the list, so bail out as we don't want to be in an infinite loop
    }
    
    switch (WifiNum) {
      case 1 : TempSSID = ssid.c_str(); break;
      case 2 : TempSSID = ssid2.c_str(); break;
      case 3 : TempSSID = ssid3.c_str(); break;
    }
  }

  log_i ("TempSSID = %s", TempSSID.c_str());
  
  if (TempSSID.length() <= 0) {
    #ifdef WIFI_DEBUG
      log_i("Nothing defined for WiFi entry %d", WifiNum);
    #endif
    ConnectToWifiNetwork(); //if not network set, go to the next
  }
  else {
    IPAddress IP;
    IPAddress Gateway;
    IPAddress Subnet;
    IP.fromString(DefaultStaticIP);
    Gateway.fromString(DefaultStaticGateway);
    Subnet.fromString(DefaultStaticSubnet);
    #ifdef WIFI_DEBUG
      log_i ("Wifi config ->");
      log_i ("Use Static IP %d", UseStaticIP);
      log_i ("IP %s", DefaultStaticIP.c_str());
      log_i ("Gateway %s", DefaultStaticGateway.c_str());
      log_i ("Subnet %s", DefaultStaticSubnet.c_str());
    #endif
    if (UseStaticIP) {
      if (!WiFi.config (IP, Gateway, Subnet)) {
        #ifdef WIFI_DEBUG
          log_i("Failed to setup wifi config");
        #endif
      }
    }
    switch (WifiNum) {
      case 1 : WiFi.begin(ssid.c_str(), Password.c_str()); break;
      case 2 : WiFi.begin(ssid2.c_str(), Password2.c_str()); break;
      case 3 : WiFi.begin(ssid3.c_str(), Password3.c_str()); break;
      case 4 : WiFi.begin(DefaultSSID.c_str(), DefaultPassword.c_str()); break;
    }
    #ifdef WIFI_DEBUG
      log_i ("Network defined for WiFi entry %d", WifiNum);
      log_i ("Connecting to %s", TempSSID.c_str());
    #endif
  }
}




void WifiSetup() {
  #ifdef WIFI_DEBUG
    log_i ("**** WIFI DEBUG ON ******");
    log_i ("Starting WIFI setup");
  #endif
  LastWifiConnectAttempt = millis();
  LastScaleConnectAttempt = LastWifiConnectAttempt; 
  LastYTWaitTime = LastWifiConnectAttempt;
  LastWifiConnectAttemptLED = LastWifiConnectAttempt;
  LastScaleConnectStatus = false;
  YTWaitTimeExceeded = false;

  //ClientNo = 0;
  
  LastEIDTime = 0;
  LastEID = "";

  WiFi.mode(WIFI_OFF);
  
  //if (SetupMode || ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK))) { 
  if ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK)) { 
    LastWifiStatus = WL_IDLE_STATUS;
    delay(10);
    digitalWrite(COMMS_PIN, CommsPin);
    
    Settings.begin("Settings", false);
    WifiName = Settings.getString(WifiNameEEAddr);
    
    DefaultSSID = Settings.getString(DefaultSSIDEEAddr, DEF_SSID);
    DefaultStaticIP = Settings.getString(DefaultStaticIPEEAddr, DEF_STATIC_IP);
    DefaultStaticGateway = Settings.getString(DefaultStaticGatewayEEAddr, DEF_STATIC_GATEWAY);
    DefaultStaticSubnet = Settings.getString(DefaultStaticSubnetEEAddr, DEF_STATIC_SUBNET);
    UseStaticIP = Settings.getBool(UseStaticIPEEAddr, true);
    DefaultScaleIP = Settings.getString(DefaultYTIPEEAddr, DEF_YTIP);
    DefaultPassword = Settings.getString(DefaultPasswordEEAddr);
    
    ssid = Settings.getString(SSIDEEAddr);
    Password = Settings.getString(PasswordEEAddr);
    ssid2 = Settings.getString(SSID2EEAddr);
    Password2 = Settings.getString(Password2EEAddr);
    ssid3 = Settings.getString(SSID3EEAddr);
    Password3 = Settings.getString(Password3EEAddr);
    Settings.end();
     
    // We start by connecting to a WiFi network
    //if (SetupMode || YTWaitTimeExceeded || (CurrentConnectionType == CONN_ACCESS_POINT)) { //0 = wifi server AP, 1 = wifi server, 2 = wifi client, 3 = BTClient, 4 = BTMaster
    if (YTWaitTimeExceeded || (CurrentConnectionType == CONN_ACCESS_POINT)) { //0 = wifi server AP, 1 = wifi server, 2 = wifi client, 3 = BTClient, 4 = BTMaster
      if (SafeMode) {
        WiFi.softAP(DEF_SSID, DEF_PASSWORD);
      }
      else {
        WiFi.softAP(WifiName.c_str(), DEF_PASSWORD);
      }
    
      delay(500);
      IPAddress TempIP;
      if (UseStaticIP) { 
        TempIP.fromString (DefaultStaticIP.c_str());
      }
      else {
        TempIP.fromString (DEF_STATIC_GATEWAY);
      }

      #ifdef WIFI_DEBUG
        log_i ("Setting up access point as follows ...");
        log_i ("IP address: %s", TempIP.toString().c_str());
      #endif
      WiFi.softAPConfig(TempIP, TempIP, DefSubnet);

      IPAddress IP = WiFi.softAPIP();
      #ifdef WIFI_DEBUG
        log_i ("AP IP address: %s", IP.toString().c_str());
        log_i ("Configured as AP %s", WifiName.c_str());
      #endif
    }
    else if ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK)) {
      //if ((CurrentConnectionType == CONN_WAIT_YT_THEN_AP) || (CurrentConnectionType == CONN_YT)) {
      //}
      WiFi.mode(WIFI_STA);
      ConnectToWifiNetwork();
    }
  }

  randomSeed(micros());

  udp.begin(UPD_PORT);
  StartConnectToScaleTask ();
}


void CheckForNewConnections (unsigned long Now){
  // Check if a new client has connected
  WiFiClient newClient = server.available();
  if (newClient) {
    //Serial.println("new client");
    // Find the first unused space
    for (int i=0 ; i<MAX_CLIENTS ; i++) {
      if (NULL == clients[i]) {
        clients[i] = new WiFiClient(newClient);//should this be "new" ... will have to check?
        MissedPings[i] = Now;
        #ifdef WIFI_DEBUG
          log_i ("Client id = %d", i);
        #endif
        break;
      }
    }
  }
}



void PingConnections (unsigned long Now){
  if (Now - LastPing > PING_TIME) {
    LastPing = Now;
    char Name[50] = "FXL-YTP001-";
    strcat (Name, GUIDStr.c_str());
    udp.beginPacket (UPD_BROADCAST_ADDR, UPD_PORT);
    udp.printf (Name);//.c_str());
    udp.endPacket ();
    #ifdef WIFI_DEBUG
      log_i ("UDP broadcast sent");
    #endif

    //now check that all clients are still connected
    for (int i=0 ; i<=MAX_CLIENTS ; i++) {
      // if (i == MAX_CLIENTS) {
      //   log_i ("Max client. Now = %lu, missing = %lu, diff = %d", Now, MissedPings[i], Now-MissedPings[i]);
      //   if (NULL == clients[i]) {
      //     log_i ("Scale Client is null");
      //   }
      // }
      if (NULL != clients[i]) {
        //MissedPings[i] = MissedPings[i] + 1;
        if ((Now - MissedPings[i] > MAX_MISSED_PING_TIME) || (!(clients[i]->connected()))) {
          #ifdef WIFI_DEBUG
            if (!(clients[i]->connected())) {
              log_i ("Client %d remotely disconnected", i);
            }
            else {
              log_i ("Client %d missed to many pings and is being disconnected", i);
            }
          #endif
          clients[i]->flush();
          clients[i]->stop();
          if (i < MAX_CLIENTS) { // don't delete scale client
            delete clients[i];
          }
          clients[i] = NULL;
        }
      }
    }

    //now ping all clients (except scales) who need a ping
    for (int i=0 ; i < MAX_CLIENTS ; i++) {
      if (NULL != clients[i]) {
        if ((Now - MissedPings[i] > PING_TIME) || (!(clients[i]->connected()))) {
          log_i("Sending ping to client : %d", i);
          clients[i]->println (PING);
        }
      }
    }
    
  }
}



void CheckForData (unsigned long Now, int ClientNo) {
  // Check whether the current client has some data
  if (NULL != clients[ClientNo]) {
    //send a command to this client if there is one in the queue
    if (GetWifiResponse(ClientNo).length() > 0){
      #ifdef WIFI_DEBUG
        log_i("Sending to client : %d : %s", ClientNo, GetWifiResponse(ClientNo).c_str());
      #endif
      clients[ClientNo]->println (GetWifiResponse(ClientNo));
      SetWifiResponse(ClientNo, "");
    }
    
    else if (GetWifiAllClientResponse(ClientNo).length() > 0){
      #ifdef WIFI_DEBUG
        log_i("Sending to all clients : %d : %s", ClientNo, GetWifiAllClientResponse(ClientNo).c_str());
      #endif
      clients[ClientNo]->println (GetWifiAllClientResponse(ClientNo));
      SetWifiAllClientResponse(ClientNo, "");
    }
   
    // If the client has some data...
    else if (clients[ClientNo]->available() ) {
      // Read the data 
      char newChar = clients[ClientNo]->read();
      #ifdef WIFI_DEBUG 
        log_i ("New Char: %c", newChar);
        //Serial.println(newChar); 
      #endif

      // If we have the end of a string
      // (Using the test your code uses)
      if (newChar == '\r') {
        #ifdef WIFI_DEBUG 
          log_i ("Recieved data : %s", inputs[ClientNo]); 
        #endif
        if (strcmp (inputs[ClientNo], PING) == 0) {
          #ifdef WIFI_DEBUG 
            log_i ("Recieved Ping"); 
          #endif
          MissedPings[ClientNo] = Now; 
          if (ClientNo == SCALE_CLIENT_IDX) { // we need to send back a ping if this is from a yt scale
            #ifdef WIFI_DEBUG 
              log_i ("Responding to ping"); 
            #endif
            clients[ClientNo]->println ("[!]");
          }
        }
        else {
          #ifdef WIFI_DEBUG 
            //log_i ("Other data recieved"); 
          #endif
          MissedPings[ClientNo] = Now; //This has been removed because Agriwebb are expecting pings regardless, fuck agriwebb
          SetWifiCommand (ClientNo, inputs[ClientNo]);
        }

        // Empty the string for next time
        inputs[ClientNo][0] = 0;
      }
      
      else if (newChar == '\n') {
        //discard the LF 
      }
      
      else if ((newChar >= ' ') && (newChar <= '~')) { //must be valid char between space and ~
        // Add it to the string
        char S[2];
        S[0] = newChar;
        S[1] = '\0';
        if (strlen(inputs[ClientNo]) < MAX_LINE_LENGTH - 2){
          strcat(inputs[ClientNo], S);
        }
        else {
          inputs[ClientNo][0] = 0;
        }
      }
    }
  }
}




void StartStopScanForYT (){
  //if (((CurrentConnectionType == CONN_WAIT_YT_THEN_AP) || (CurrentConnectionType == CONN_YT)) && (!YTWaitTimeExceeded)) { 
  if ((CurrentConnectionType == CONN_WAIT_YT_THEN_AP) && (!YTWaitTimeExceeded)) { 
    RestartingWifi = true;
    while (RestartingWifi) delay (10);
    #ifdef WIFI_DEBUG
      log_i ("Stopping the scan for YardsTech Wifi ...");
    #endif
    //YTWaitTimeExceeded = true;
    CurrentConnectionType = CONN_ACCESS_POINT;

    WifiSetup ();
    StartWifiLoopTask ();
  }
  
  else if ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_ACCESS_POINT)) { 
    RestartingWifi = true;
    while (RestartingWifi) delay (10);
    #ifdef WIFI_DEBUG
      log_i ("About to start a rescan for YardsTech Wifi ...");
    #endif
    YTWaitTimeExceeded = false;
    CurrentConnectionType = CONN_WAIT_YT_THEN_AP;

    WifiSetup ();
    StartWifiLoopTask ();
  }
}


void WifiLoop () {
  unsigned long Now = millis();
  //any wifi mode i.e. not set to bluetooth mode
  if ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK)) {
    int WifiStatus = WiFi.status();

    #ifdef WIFI_DEBUG
//      IPAddress IP = WiFi.softAPIP();
//      Serial.print("AP IP address: ");
//      Serial.println(IP);      
//      Serial.print("WiFi Status changed from ");
//      Serial.println(WiFi.channel());
      //Serial.print(" to ");
      //Serial.println(WifiStatus);
    #endif
    
    if (YTWaitTimeExceeded || (CurrentConnectionType == CONN_ACCESS_POINT)) {
      //there seems to be a bug that if in ap mode and a BT client connects it shuts down the wifi ... this just sets it up again. 
      WifiStatus = WL_CONNECTED; //force connected when in access point mode
    }
    
    //Are we not connected to a wifi network yet?
    if (WifiStatus != WL_CONNECTED) {
      if (Now - LastWifiConnectAttemptLED > 250) {
        LastWifiConnectAttemptLED = Now;
        CommsPin = !CommsPin;
        digitalWrite(COMMS_PIN, CommsPin);     
      }
      
      if (Now - LastWifiConnectAttempt > WIFI_CONNECT_TIMEOUT) {
        if ((CurrentConnectionType == CONN_WAIT_YT_THEN_AP) && (Now - LastYTWaitTime > YT_WAIT_TIMEOUT)) {
          #ifdef WIFI_DEBUG
            log_i ("YT network wait time exceeded. Going back to wifi setup. WifiStatus = %d", WifiStatus);
          #endif
          YTWaitTimeExceeded = true;
          WifiSetup();
        }
        else {    
          LastWifiConnectAttempt = Now;
          ConnectToWifiNetwork();
        }
      }
    }
    
    //Has the wifi status just changed to connected? 
    else if (LastWifiStatus != WifiStatus) {
      char MAC[18] = "";

      BuzzOn (BUZZ_WIFI_CONNECT, 1, QUIET_WIFI_CONNECT, true);
      
      #ifdef WIFI_DEBUG
        log_i ("WiFi connected");
        log_i ("IP address: %s", WiFi.localIP().toString().c_str());
      #endif
      
      WiFi.macAddress().toCharArray (MAC, 18);
      digitalWrite(COMMS_PIN, true);     
      //Serial.println(MAC);
      
      if (strcmp (GUIDStr.c_str(), MAC) != 0) { //guid hasn't been set yet
        log_i ("Blank or invalid device ID. A new one getting saved.");
        GUIDStr = String (MAC);
        Settings.begin("Settings", false);
        Settings.putString(GUIDEEAddr, GUIDStr);
        Settings.end();
      }
  
      //if (SetupMode || ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK))) {
      if ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK)) {
        server.begin();
      }
    }

    //are we trying to connect to YardsTech wifi and have not yet exceeded the wait time?
    else if ((CurrentConnectionType == CONN_WAIT_YT_THEN_AP) && (!YTWaitTimeExceeded) && (WifiStatus != WL_CONNECTED) ) {
      if (Now - LastYTWaitTime > YT_WAIT_TIMEOUT) {
        #ifdef WIFI_DEBUG
          log_i ("YT wait time exceeded. Going back to wifi setup.");
        #endif
        YTWaitTimeExceeded = true;
        BuzzOn (BUZZ_WIFI_CONNECT, 1, QUIET_WIFI_CONNECT, true);
        WifiSetup();
      }    
    }

    //we are connected, so run through all of the stuff we have to do    
    else {//here is where the real work happens
      //if (SetupMode || ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK))) {
      if ((CurrentConnectionType >= CONN_WAIT_YT_THEN_AP) && (CurrentConnectionType <= CONN_EXT_NETWORK)) {
  
        int Client = 0;
        while (Client <= TOTAL_CLIENTS) {
          if (Client == MAX_CLIENTS + 1){
            CheckForNewConnections (Now);
          }
          else if (Client == TOTAL_CLIENTS){
            PingConnections (Now);
          }
          else if (Client <= MAX_CLIENTS){
            CheckForData (Now, Client);
          }
          else {
            //Client = -1;
          }

          Client = Client + 1;
        }
          
        LastYTWaitTime = Now;
      }
    }
    

    //check for scale connectivity
    if (!ScaleClient.connected()) {
      if (LastScaleConnectStatus) { //was this connected before, we must have disconncted
        BuzzOn (BUZZ_WIFI_CONNECT, 3, QUIET_WIFI_CONNECT, true); //we have disconnected
        #ifdef WIFI_DEBUG
          log_i ("Disconnected from Scale");
        #endif
        LastScaleConnectAttempt = 0; // this will force a reconnect on the next if statement
      }
      LastScaleConnectStatus = false;

      // ScanForScales(); //see if we can find the scales using UPD

      // if (Now - LastScaleConnectAttempt > SCALE_CONNECT_TIMEOUT) {
      //   #ifdef WIFI_DEBUG
      //     log_i ("Trying to connect to scale.");
      //   #endif

      //   LastScaleConnectAttempt = Now;
      //   ScaleClient.connect(DefaultScaleIP.c_str(), TCP_PORT, 250);
      // }
    }
    //scale just connected
    else if (LastScaleConnectStatus != ScaleClient.connected()) {
      LastScaleConnectStatus = ScaleClient.connected();
      if (LastScaleConnectStatus) {
        BuzzOn (BUZZ_WIFI_CONNECT, 2, QUIET_WIFI_CONNECT, true);
        #ifdef WIFI_DEBUG
          log_i ("Connected to Scale");
        #endif
        clients[SCALE_CLIENT_IDX] = &ScaleClient;
        MissedPings[SCALE_CLIENT_IDX] = Now;
        SetWifiResponse(SCALE_CLIENT_IDX, "[WCE1]"); //enable weight events
      }
    }

    LastWifiStatus = WifiStatus;
  }
}




void WifiLoopTask (void * parameter) {
  log_i("WifiTask Started");
  while (!OTAUpdating && !ExternalPoweredOn && !PoweringDown && !RestartingWifi) {
    vTaskDelay(1);

    WifiLoop ();
  }

  RestartingWifi = false;

  log_i("WifiTask stopped!");
  vTaskDelete (NULL);
}


void StartWifiLoopTask (){
  xTaskCreate(
                  WifiLoopTask,          /* Task function. */
                  "WifiLoopTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  HIGHEST_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}



