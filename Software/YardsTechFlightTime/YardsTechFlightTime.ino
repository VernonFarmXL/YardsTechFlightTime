#include <math.h>
#include "Crc16.h"
#include "arduino.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
#include <WiFi.h>

#include "BluetoothSerial.h"
#include "YTGlobals.h"
#include "YTBuzz.h"
#include "YTBatt.h"
#include "YTOTA.h"
#include "YTBTMaster.h"
#include "YTCommands.h"
#include "YTWifi.h"
#include "YTBTClient.h"
#include "LD2450.h"

 
#define TIME_BETWEEN_MODES 3000 //the time to wait to switch between modes during turn on
#define QUARTER_TIME_BETWEEN_MODES TIME_BETWEEN_MODES/4 //the time to wait to switch between modes during turn on
#define SETUP_MODE_FLASH_TIME 1000 // time between led flash changes in setup mode
#define TAG_READ_FLASH_TIME 50 //
#define PAUSED_FLASH_WAIT_TIME 1500
#define PAUSED_FLASH_TIME 100
#define READ_FLASH_TIME 100
#define SAFE_MODE_FLASH_TIME 80 // time between led flash changes in safe mode
#define BT_CONNECT_FLASH_TIME 80 // flash time if trying to connect to bt client
#define BT_DISABLED_FLASH_TIME 1000 // flash time if bt is disabled


unsigned long LastPwrBtnPress = 0;
unsigned long LastReadBtnPress = 0;
unsigned long LastActLEDTimer = 0;
unsigned long LastCommsLEDTimer = 0;
bool WasInBTConnect = false;
unsigned long LastExtPowerCheck = 0;
unsigned long PwrButtonPressTime = 0;
unsigned long IgnoreReadBtnUntil = 0;
bool ExtPowerConnected = false;


int ProcessNum;
int TagReadFlagCount = 0;

#define RXD2 16
#define TXD2 17

//UART Serial2(0, 1, NC, NC);
//HardwareSerial Serial2(2);

// SENSOR INSTANCE
LD2450 ld2450;

#define TARGET_ROWS 50
#define NUM_TARGETS 3

LD2450::RadarTarget targetArray[TARGET_ROWS][NUM_TARGETS];
unsigned long targetArrayTimes[TARGET_ROWS][NUM_TARGETS];
uint8_t targetIdx [NUM_TARGETS];
unsigned long targetMoving [NUM_TARGETS];
uint16_t maxTargetDistance = 2000; // in mm i.e. 2m
uint16_t minTargetSpeed = 10; //10cm/s
uint16_t noActivityTimeout = 6000; // 6 seconds


 
//=======================================================================
//                    Power on setup
//=======================================================================
void setup() {
  PoweringDown = false;
  
  pinMode(COMMS_PIN, OUTPUT);     
  pinMode(ACTIVITY_PIN, OUTPUT);
  pinMode(POWER_BTN, INPUT);
  pinMode(EXT_POWER_INPUT, INPUT);
  pinMode(EXT_POWER_PIN, OUTPUT);
  pinMode(POWER_ON_OUTPUT, OUTPUT);

  //pinMode(READ_TAG_INPUT, INPUT);

  Serial.begin(115200);
  Serial2.begin(256000, SERIAL_8N1, RXD2, TXD2);
  while (!Serial2) {

  }
  ld2450.begin(Serial2, true);
   
  #ifdef MAIN_DEBUG
  log_i ("**** MAIN DEBUG ON ******");

  if(!ld2450.waitForSensorMessage()){
    log_i("SENSOR CONNECTION SEEMS OK");
  }
  else{
    log_i("SENSOR TEST: GOT NO VALID SENSORDATA - PLEASE CHECK CONNECTION!");
  }
  #endif

    
  Settings.begin("Settings", false);  
  DataModNo = Settings.getInt(DataModNoEEAddr);//Checks to see if this is the first time we have started and if we need to do a factor reset
  log_i("Data mod no = %d", DataModNo);
  Settings.end();  
  
  FactoryReset (DataModNo);
  
  Settings.begin("Settings", false);  
  GUIDStr = Settings.getString(GUIDEEAddr);//loads guid from flash
  SerialNo = Settings.getString(SerialNoEEAddr);//Get serial no from flash
  ConnectionType = Settings.getInt(ConnectionTypeEEAddr);
  CurrentConnectionType = ConnectionType;
  log_i("Connection type = %d", ConnectionType);
  if ((ConnectionType < CONN_WAIT_YT_THEN_AP) || (ConnectionType > CONN_BT_MASTER)) {
    ConnectionType = CONN_WAIT_YT_THEN_AP; //this should probably be CONN_ACCESS_POINT 
  }
  Settings.end();

  log_i("Device ID = %s", GUIDStr.c_str());
    
  log_i("Battery setup");
  BatterySetup ();
  log_i("Turning battery on");
  digitalWrite (POWER_ON_OUTPUT, HIGH); //keep the power on for the moment
  
  delay (500); //wait 500msec to see if button still pressed
    
  if (digitalRead (POWER_BTN) != LOW){ //if power button released then go back to sleep. Power button may just have been knocked.
    if (digitalRead (EXT_POWER_INPUT) == HIGH){
        ExternalPoweredOn = true; // externally powered, so keep powered in the background
        log_i("Externally powered");
    }
    else {
      log_i("Power button not pressed. Going to sleep");
      GoToSleep ();
      return;
    }
  }

  if (!ExternalPoweredOn) { //user has turned on the unit
    log_i("All systems go ...");
    //All systems go ...
    
    log_i("Sending activity LED pin high"); 
    digitalWrite(ACTIVITY_PIN, HIGH);

    log_i("Buzzer setup");
    BuzzSetup ();
    BuzzOn (BUZZ_POWER_ON, 1, 0, true);
  
    SetupMode = false;
    SafeMode = false;
    
    //Check is user still holding button after another TIME_BETWEEN_MODES seconds ... if so, go into setup mode
    delay (QUARTER_TIME_BETWEEN_MODES);
    BuzzOff ();
    if (digitalRead (POWER_BTN) == LOW){ 
      delay (QUARTER_TIME_BETWEEN_MODES);
    }
    if (digitalRead (POWER_BTN) == LOW){ 
      delay (QUARTER_TIME_BETWEEN_MODES);
    }
    if (digitalRead (POWER_BTN) == LOW){ 
      delay (QUARTER_TIME_BETWEEN_MODES);
    }
    if (digitalRead (POWER_BTN) == LOW){ 
      BuzzOn (BUZZ_POWER_ON, 1, 0, true);
      log_i("Going into setup mode");
      SetupMode = true;
      CurrentConnectionType = CONN_ACCESS_POINT;
      digitalWrite(ACTIVITY_PIN, LOW);
      digitalWrite(COMMS_PIN, HIGH);
    }
    
    delay (QUARTER_TIME_BETWEEN_MODES);
    BuzzOff ();
    //Check is user still holding button after another TIME_BETWEEN_MODES seconds ... if so, go into safe mode
    if (digitalRead (POWER_BTN) == LOW){ 
      delay (QUARTER_TIME_BETWEEN_MODES);
    }
    if (digitalRead (POWER_BTN) == LOW){ 
      delay (QUARTER_TIME_BETWEEN_MODES);
    }
    if (digitalRead (POWER_BTN) == LOW){ 
      delay (QUARTER_TIME_BETWEEN_MODES);
    }
    if (digitalRead (POWER_BTN) == LOW){ 
      BuzzOn (BUZZ_POWER_ON, 1, 0, true);
      log_i("Going into safe mode");
      SafeMode = true;
      digitalWrite(ACTIVITY_PIN, HIGH);
    }
    
    UserPoweredOn = true;
    //ProcessNum = 0;  
  
    if (!SafeMode) {
    }
    // log_i("Command setup");
    // CommandsSetup ();
    // log_i("Wifi setup");
    // WifiSetup ();

    if (!SafeMode) {
      // log_i("BT Master setup");
      // SetupBTMaster (false);
      // log_i("BT Client setup");
      // SetupBTClient ();
      log_i("Starting Tasks");
      StartNormalTasks();
    }
    else {
      StartSafeModeTasks();
    }
  } 
  else {
    StartExternalPoweredOnTasks();//StartBatteryTask();    
  }

  digitalWrite (EXT_POWER_PIN, LOW); //initialise the ext power led off
}



void PowerButtonLoop () {
  unsigned long Now = millis();
  int PinVal = digitalRead (POWER_BTN);
  if (PinVal == LOW){
    if (LastPwrBtnPress == 0) {
      LastPwrBtnPress = Now;
    }
    else if ((Now - LastPwrBtnPress > 500) && ExternalPoweredOn){ //externally power on and now user is turning the unit on
      PwrButtonPressTime = 0;
      log_i("Was externally powered and now user switching on"); 
      ExternalPoweredOn = false;
      RestartModule (false);
    }
    else if (Now - LastPwrBtnPress > 2500){ //must hold for 2.5 seconds to turn off
      if (!PoweringDown) {
        PoweringDown = true;
        CommsPin = LOW;
        BuzzOn (BUZZ_POWER_OFF, 1, 0, true);
        //delay(1000);
        //BuzzOff ();
        log_i("Powering down"); 
        GoToSleep ();
      }
    }

    PwrButtonPressTime = Now - LastPwrBtnPress; 
  }
  
  else {
    LastPwrBtnPress = 0;
    if (PwrButtonPressTime > 0) {      
      //this is only used when wifi is in wait for YT mode and you want to rescan for it
      if ((PwrButtonPressTime > 50) && (PwrButtonPressTime < 500) && UserPoweredOn){ //unit on and now user wants to start/stop scan for YT device/network
        //log_i("Unit on, rescanning for YT network/device");
        StartStopScanForYT ();
      }
      
      PwrButtonPressTime = 0;
    }
  }
}



void CheckForExternalPower () {
  unsigned long Now = millis();
  if (LastExtPowerCheck == 0) {
    LastExtPowerCheck = Now;
  }
  else if (Now - LastExtPowerCheck > 500) { //check every 500ms
    LastExtPowerCheck = Now;
    if ((!ExtPowerConnected) && (digitalRead (EXT_POWER_INPUT) == HIGH)){//check if external power connected and show LED if it is
      digitalWrite (EXT_POWER_PIN, HIGH);
      digitalWrite (POWER_ON_OUTPUT, HIGH); //make sure power stays on
      ExtPowerConnected = true;
      log_i("Detected that external power has been connected"); 
    }
    else if ((ExtPowerConnected) && (digitalRead (EXT_POWER_INPUT) == LOW)) {
      digitalWrite (EXT_POWER_PIN, LOW);
      ExtPowerConnected = false;
      log_i("Detected that external power has been disconnected"); 
      if (ExternalPoweredOn) {
        log_i("Not user powered on, so turn off"); 
        GoToSleep (); //if just siting on the charger and charger disconnected then power down
      }
    }  
  }
}



void ActivityLEDTask(void * parameter) {  
//void ActivityLEDLoop () {
  int count = 0;
  unsigned long LastActLEDWaitTimer = 0;
  while (!OTAUpdating && !ExternalPoweredOn && !PoweringDown) {
    vTaskDelay(pdMS_TO_TICKS(25));
    unsigned long Now = millis();
    if (TagReadFlag) {
      if (Now - LastActLEDTimer > TAG_READ_FLASH_TIME){ //flash LED fast
        LastActLEDTimer = Now;
        count = count + 1;
        if (count > 6) {
          TagReadFlag = false;
          count = 0;
          ActivityLED = false; //turn it on
        }
        ActivityLED = !ActivityLED;
        digitalWrite(ACTIVITY_PIN, ActivityLED);
      }
    }
    // else if (PausedDueToScale){
    //   if (Now - LastActLEDWaitTimer > PAUSED_FLASH_WAIT_TIME){
    //     if (Now - LastActLEDTimer > PAUSED_FLASH_TIME){ //flash LED fast
    //       LastActLEDTimer = Now;
    //       count = count + 1;
    //       if (count > 4) {
    //         count = 0;
    //         ActivityLED = false; //turn it on
    //         LastActLEDWaitTimer = Now;
    //       }
    //       ActivityLED = !ActivityLED;
    //       digitalWrite(ACTIVITY_PIN, ActivityLED);
    //     }
    //   }
    // }
    else if (SafeMode) {
      if (Now - LastActLEDTimer > SAFE_MODE_FLASH_TIME){ //flash LED if in setup mode
        LastActLEDTimer = Now;
        ActivityLED = !ActivityLED;
        digitalWrite(ACTIVITY_PIN, ActivityLED);
      }
    }
    else if (SetupMode) {
      if (Now - LastActLEDTimer > SETUP_MODE_FLASH_TIME){ //flash LED if in setup mode
        LastActLEDTimer = Now;
        ActivityLED = !ActivityLED;
        digitalWrite(ACTIVITY_PIN, ActivityLED);
      }
    }
    else {
      // ActivityLED = true;
      // digitalWrite(ACTIVITY_PIN, ActivityLED);
      if (Now - LastActLEDWaitTimer > READ_FLASH_TIME){
        if (Now - LastActLEDTimer > READ_FLASH_TIME){ //flash LED fast
          LastActLEDTimer = Now;
          ActivityLED = !ActivityLED;
          digitalWrite(ACTIVITY_PIN, ActivityLED);
        }
      }
    }
    // else if (_ContinuousReads) {
    //   if (Now - LastActLEDTimer > SAFE_MODE_FLASH_TIME){ //flash LED fast
    //     LastActLEDTimer = Now;
    //     ActivityLED = !ActivityLED;
    //     digitalWrite(ACTIVITY_PIN, ActivityLED);
    //   }
    // }
  }
  digitalWrite(ACTIVITY_PIN, LOW);
  log_i("Activity LED Task stopped!");
  vTaskDelete (NULL);
}

void StartActivityLEDTask (){
  xTaskCreate(
                  ActivityLEDTask,          /* Task function. */
                  "ActivityLEDTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  LOW_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}




void BTCommsLEDTask(void * parameter) {  
//void BTCommsLEDLoop () {  
  while (!OTAUpdating && !ExternalPoweredOn && !PoweringDown) {
    vTaskDelay(pdMS_TO_TICKS(25));
    unsigned long Now = millis();
    #ifdef LOCAL_BT_IMPLMENTATION
      if (((CurrentConnectionType == CONN_BT_CLIENT) && !LastBTClientConnected && !SetupMode && !SafeMode) 
          || (!SerialBT.ConnectDone()) 
          || (!SerialBT.DiscoverDone())) {
        WasInBTConnect = true;
        if (Now - LastCommsLEDTimer > BT_CONNECT_FLASH_TIME){ //flash comms LED if in bluetooth discovery
          LastCommsLEDTimer = Now;
          CommsPin = !CommsPin;
          digitalWrite(COMMS_PIN, CommsPin);     
        }
      }
      else if (WasInBTConnect) {
        WasInBTConnect = false;
        CommsPin = HIGH;
        digitalWrite(COMMS_PIN, CommsPin);     
      }
      else if (BtDisabled) {
        if (Now - LastCommsLEDTimer > BT_DISABLED_FLASH_TIME){ //flash comms LED if in bluetooth disabled
          LastCommsLEDTimer = Now;
          CommsPin = !CommsPin;
          digitalWrite(COMMS_PIN, CommsPin); 
        }    
      }
    #endif
  }
  digitalWrite(COMMS_PIN, LOW);     
  log_i("BT LED Task stopped!");
  vTaskDelete (NULL);
}


void StartBTCommsLEDTask (){
  xTaskCreate(
                  BTCommsLEDTask,          /* Task function. */
                  "BTCommsLEDTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  LOW_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}


void LD2450Task(void * parameter) {  
  while (!OTAUpdating && !ExternalPoweredOn && !PoweringDown) {
    vTaskDelay(pdMS_TO_TICKS(1));
    // READ FUNCTION MUST BE CALLED IN LOOP TO READ THE INCOMMING DATA STREAM
    // RETURNS -1 or -2 as error flag and 0 to getSensorSupportedTargetCount() if valid targets found
    const int sensor_got_valid_targets = ld2450.read();
    if (sensor_got_valid_targets > 0)
    {
      unsigned long startTime = millis();

      /*
      PRINT DEBUG DATA STREAM LIKE THIS: 
      TARGET ID=1 X=-19mm, Y=496mm, SPEED=0cm/s, RESOLUTION=360mm, DISTANCE=496mm, VALID=1
      TARGET ID=2 X=-1078mm, Y=1370mm, SPEED=0cm/s, RESOLUTION=360mm, DISTANCE=1743mm, VALID=1
      TARGET ID=3 X=0mm, Y=0mm, SPEED=0cm/s, RESOLUTION=0mm, DISTANCE=0mm, VALID=0
      */
      //Serial.print(ld2450.getLastTargetMessage());


      // GET THE DETECTED TARGETS
      // TARGET RANGE CAN BE FROM 0 TO ld2450.getSensorSupportedTargetCount(), DEPENDS ON SENSOR HARDWARE. REFER TO LD2450 DATASHEET
      for (int i = 0; i < NUM_TARGETS; i++)
      {
        const LD2450::RadarTarget result_target = ld2450.getTarget(i);

        //check if target has finished moving
        if (targetMoving[i] && ((abs(result_target.speed) == 0) || abs(result_target.distance) > maxTargetDistance)) {
          targetArray [targetIdx[i]][i] = result_target;
          targetArrayTimes [targetIdx[i]][i] = millis();
          //report speed details
          log_i("Target Detected %d in %d steps", i, targetIdx[i] + 1);
          for (int j = 0; j <= targetIdx[i]; j++) {
            log_i("%d %dms X=%dmm, Y=%dmm, S=%dcm/s, D=%dmm, R=%dmm", 
              j,
              targetArrayTimes[j][i] - targetMoving[i],
              targetArray[j][i].x,
              targetArray[j][i].y,
              targetArray[j][i].speed,
              targetArray[j][i].distance,
              targetArray[j][i].resolution
              );
          }
          targetMoving[i] = 0;
          targetIdx[i] = 0;
        }
        else if (targetMoving[i]) {
          if ((millis() - targetMoving[i] > noActivityTimeout) 
            || (abs(result_target.speed) == 0)){
            targetIdx[i] = 0;
            targetMoving[i] = 0;
          }
          else {
            targetArray [targetIdx[i]][i] = result_target;
            targetArrayTimes [targetIdx[i]][i] = millis();
            targetIdx[i] = targetIdx[i] + 1;
          }
        }
        //has target started moving
        else if (!targetMoving[i] && (abs(result_target.speed) > minTargetSpeed) && abs(result_target.distance) <= maxTargetDistance) {
          targetArray [targetIdx[i]][i] = result_target;
          targetMoving[i] = millis();
          targetArrayTimes [targetIdx[i]][i] = targetMoving[i];
          targetIdx[i] = targetIdx[i] + 1;
        }
      }

      //log_i ("%dms", millis() - startTime);
    };
  }
  log_i("LD2450 Task stopped!");
  vTaskDelete (NULL);
}





void StartLD2450Task (){
  xTaskCreate(
                  LD2450Task,          /* Task function. */
                  "LD2450Task",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  HIGHEST_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}



void StartNormalTasks (){
  StartActivityLEDTask ();
  StartBuzzTask ();
  //StartWifiLoopTask ();
  StartBatteryTask();
  //StartCommandsTask ();
  //StartBTMasterTask ();
  //StartBTCommsLEDTask ();
  StartLD2450Task();
}


void StartSafeModeTasks (){
  StartActivityLEDTask ();
  StartBuzzTask ();
  StartWifiLoopTask ();
  StartBatteryTask();
  StartCommandsTask ();
}


void StartExternalPoweredOnTasks (){
  StartBatteryTask();
}




//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {
  Serial.println("Started flight time code");

  while(1)
  {
    if (!OTAUpdating && !ExternalPoweredOn && !PoweringDown && !SafeMode) {
      //BTClientLoop ();
    }

    
    PowerButtonLoop();
    CheckForExternalPower ();
  }
}
