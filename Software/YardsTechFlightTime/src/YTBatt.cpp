#include "arduino.h"
#include "YTGlobals.h"
#include "iirFilter.h"

#define ZERO_BATT_VOLTS 9.4 //6.4 //the voltage below which the battery is assumed to be fully discharged
#define ZERO_BATT_VOLTS_ADC 927 //557 //adc reading at zero batt volts
#define FULL_BATT_VOLTS 11.8 //11.75 //11.5 //11.95 //12.2 //7.9 //8.4 //the voltage above which the battery is assumed to be fully charged
#define FULL_BATT_VOLTS_ADC 1203 //1198 //1169 //1221 //1250 //780 for 8.4 //adc reading at full batt volts
#define FULL_CHARGE_VOLTS 13.5 //10.5 //9.0//9.4//10.00 //12.6 for 2S unit and 12.4 3S unit //the voltage above which the battery is assumed to be fully charged
#define FULL_CHARGE_RANGE 0.01 //at full charge the current batt volt must be within 1% of last batt volts 
float BATT_VOLT_RANGE =  FULL_BATT_VOLTS - ZERO_BATT_VOLTS; //1.5 for 2S unit and 2.8 for 3S unit //the battery voltage range
int BATT_ADC_RANGE = FULL_BATT_VOLTS_ADC - ZERO_BATT_VOLTS_ADC;
const int NO_OF_BATT_VALS = 10;
//RTC_DATA_ATTR int BattVals[NO_OF_BATT_VALS];
IIRFilter BattVals;
//RTC_DATA_ATTR int BattValIdx = 0;
//unsigned long LastBatteryCheck = 0;
float BattVoltOffset = 0; //offset applied to the battery voltage for correction purposes
float BattVolts = 0;
//RTC_DATA_ATTR float LastBattVolts = 0;
int BattPercent = 0;




void ResetBatteryVoltage (void) {
    if ((BattVoltOffset < -5) || (BattVoltOffset > 5)){
      BattVoltOffset = 0;
      Settings.begin("Settings", false);
      Settings.putFloat(BattVoltOffsetEEAddr, 0);  
      Settings.end();
      log_fxl("Reset battery voltage offset to 0");
    }
    //BattValIdx = 0;
    // for (int i = 0; i < NO_OF_BATT_VALS; i++) {
    //   BattVals[i] = 0;
    // }
}


void BatterySetup (void) {
  #ifdef BATT_DEBUG
  log_fxl ("**** BATT DEBUG ON ******");
  #endif

  pinMode(BATT_VOLTS, INPUT); 
  pinMode(BATT_CHARGED_PIN, OUTPUT); 

  Settings.begin("Settings", false);
  BattVoltOffset = Settings.getFloat(BattVoltOffsetEEAddr);
  Settings.end();
  log_fxl("Batt Voltage Offset = %f", BattVoltOffset);
  BattVals.setAlphaValue (0.75);
  ResetBatteryVoltage (); //clear the current battery voltage settings
}


void BatteryTask(void * parameter) {  
  log_fxl ("Starting battery task");
  while (!OTAUpdating && !PoweringDown) {
    vTaskDelay(pdMS_TO_TICKS(100));
    int X = analogRead (BATT_VOLTS);
    BattVals.processNextStep(X);
    float BattADC = BattVals.getLastValue(); // get average
    float BattBeforeOffset = (BattADC - ZERO_BATT_VOLTS_ADC) / BATT_ADC_RANGE * BATT_VOLT_RANGE + ZERO_BATT_VOLTS; //get unoffseted voltage
    BattVolts = BattBeforeOffset + BattVoltOffset; //offset voltage 
    
    BattPercent = (int)(((BattVolts - ZERO_BATT_VOLTS) / BATT_VOLT_RANGE) * 100);
    #ifdef BATT_DEBUG
      log_fxl("Batt ADC = %f", BattADC);
      log_fxl("Batt Percent = %d", BattPercent);
      log_fxl("Batt Voltage = %f", BattBeforeOffset);
      log_fxl("Batt Voltage Offset = %f", BattVolts);
    #endif

    if (BattPercent > 100) {
      BattPercent = 100;
    }
    else if (BattPercent < 1) {
      BattPercent = 1;
    }
  
    if (BattVolts >= FULL_CHARGE_VOLTS) {
      digitalWrite (BATT_CHARGED_PIN, HIGH);
    }
    else {
      digitalWrite (BATT_CHARGED_PIN, LOW);
    }
  }
  
  digitalWrite (BATT_CHARGED_PIN, LOW);
  log_fxl("Battery Task stopped!");
  vTaskDelete (NULL);
}

void StartBatteryTask (){
  xTaskCreate(
                  BatteryTask,          /* Task function. */
                  "BatteryTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  LOW_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}
