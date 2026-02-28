#include "YTGlobals.h"
#include <mutex>


std::mutex BuzzMutex;
const int BuzzerChannel = 1;
int BuzzerFreq = 2048;//2048Hz
const int BuzzerResolution = 5; //must be 1 ....range is 1-14 bits (1-20 bits for ESP32)
unsigned long LastBuzzerOn = 0;
//const int BuzzerDuration = 500; //500ms
bool BuzzerOn;
int BuzzTime; 
int BuzzCount; 
int QuietTime;
bool lastNotaTag = true;
boolean QuietMode = false;



void BuzzOff () {
  std::lock_guard<std::mutex> guard(BuzzMutex);
  BuzzerOn = false;
  LastBuzzerOn = 0;
  #if ESP_IDF_VERSION_MAJOR >= 5
    ledcWriteChannel(BuzzerChannel, 0); 
  #else
    ledcWrite(BuzzerChannel, 0);
  #endif
  #ifdef BUZZ_DEBUG
    log_i("Buzzer OFF buzztime=%d, buzzcount=%d, quiettime=%d", BuzzTime, BuzzCount, QuietTime);
  #endif
}


//BuzzTime and QuietTime in ms
void BuzzOn (int _BuzzTime, int _BuzzCount, int _QuietTime, bool _notaTag) {//_onOff means powering on or off ... always beep and buzz
  std::lock_guard<std::mutex> guard(BuzzMutex);
  lastNotaTag = _notaTag;
  if (LastBuzzerOn == 0) {
    BuzzTime = _BuzzTime;
    BuzzCount = _BuzzCount * 2; //1 count for time on and 1 count for time off
    QuietTime = _QuietTime;
    
    long Now = millis();
    LastBuzzerOn = Now;
    if (!QuietMode || _notaTag) {
      #if ESP_IDF_VERSION_MAJOR >= 5
        ledcWriteChannel(BuzzerChannel, 16); 
      #else
        ledcWrite(BuzzerChannel, 16);
      #endif
    } 

    BuzzerOn = true;
    #ifdef BUZZ_DEBUG
      log_i("Buzzer ON buzztime=%d, buzzcount=%d, quiettime=%d", BuzzTime, BuzzCount, QuietTime);
    #endif
  }
}



void BuzzWait () {
  std::lock_guard<std::mutex> guard(BuzzMutex);
  if (LastBuzzerOn == 0) {
    long Now = millis();
    LastBuzzerOn = Now;
    BuzzerOn = true;
    #ifdef BUZZ_DEBUG
      log_w("Buzzer wait buzztime=%d, buzzcount=%d, quiettime=%d", BuzzTime, BuzzCount, QuietTime);
    #endif
  }
}



void BuzzSetup (void) {
  #ifdef BUZZ_DEBUG
  log_i ("**** BUZZ DEBUG ON ******");
  #endif
    
  pinMode(BUZZER_PIN,OUTPUT);
  
  #if ESP_IDF_VERSION_MAJOR >= 5
    float frq = ledcAttachChannel (BUZZER_PIN, BuzzerFreq, BuzzerResolution, BuzzerChannel);
  #else
    float frq = ledcSetup(BuzzerChannel, BuzzerFreq, BuzzerResolution);
    ledcAttachPin(BUZZER_PIN, BuzzerChannel);
  #endif
  log_i("Buzzer freq: %f", frq);
  
  Settings.begin("Settings", false);  
  QuietMode = Settings.getBool(QuietEEAddr);
  BuzzerFreq = Settings.getInt(BuzzFreqEEAddr, BuzzerFreq);
  Settings.end();
}


void BuzzTask(void * parameter) {  
//void BuzzLoop() {
  while (!OTAUpdating && !ExternalPoweredOn) {// && !PoweringDown) {
    vTaskDelay(pdMS_TO_TICKS(25));
    if (BuzzerOn) {
      if (LastBuzzerOn > 0) {
        unsigned long Now = millis();
        if ((BuzzCount % 2 == 0) && (Now - LastBuzzerOn > BuzzTime)) { //even buzzcount so must be buzz time
          #ifdef BUZZ_DEBUG
            log_i("Buzzer on check buzztime=%d, buzzcount=%d, quiettime=%d", BuzzTime, BuzzCount, QuietTime);
          #endif
          BuzzOff ();
          BuzzCount = BuzzCount - 1;
          if (BuzzCount > 1) { //only bother to wait if there will be another buzzer on time
            BuzzWait();
          }
        }
        else if ((BuzzCount % 2 == 1) && (Now - LastBuzzerOn > QuietTime)) { //odd buzzcount so must be quiet time
          #ifdef BUZZ_DEBUG
            log_i("Buzzer quiet check buzztime=%d, buzzcount=%d, quiettime=%d", BuzzTime, BuzzCount, QuietTime);
          #endif
          BuzzCount = BuzzCount - 1;
          LastBuzzerOn = 0;
          BuzzOn(BuzzTime, BuzzCount / 2, QuietTime, lastNotaTag);
        }
      }
    }
  }
  BuzzOff();
  log_i("Buzz Task stopped!");
  vTaskDelete (NULL);
}

void StartBuzzTask (){
  xTaskCreate(
                  BuzzTask,          /* Task function. */
                  "BuzzTask",        /* String with name of task. */
                  10000,            /* Stack size in bytes. */
                  NULL,             /* Parameter passed as input of the task */
                  NORMAL_PRI,               /* Priority of the task. */
                  NULL           /* Task handle. */
                  );            
}
