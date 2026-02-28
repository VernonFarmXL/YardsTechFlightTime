#ifndef _YTBUZZ_H
#define _YTBUZZ_H

#define BUZZ_TAG_READ 500 //ms for a tag read
#define BUZZ_POWER_ON 500 //ms for a power on
//#define QUIET_POWER_ON 50 //ms between beeps for a power on
#define BUZZ_POWER_OFF 250 //ms for a power off
#define BUZZ_WIFI_CONNECT 250 //ms for when wifi connects
#define QUIET_WIFI_CONNECT 50 //ms for when
#define BUZZ_BT_CONNECT 250 //ms for when BT connects
#define QUIET_BT_CONNECT 50 //ms for when
#define BUZZ_CONT_READ 100 //ms for when continuous reading turned off/on
#define QUIET_CONT_READ 50 //ms for when

extern boolean QuietMode;
extern int BuzzerFreq;



void BuzzSetup (void);
void StartBuzzTask (); 

void BuzzOn (int _BuzzTime, int _BuzzCount, int _QuietTime, bool _notaTag);//_onOff means powering on or off ... always beep and buzz
void BuzzOff ();


#endif
