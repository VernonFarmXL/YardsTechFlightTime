#ifndef _YTBATT_H
#define _YTBATT_H

extern float BattVoltOffset;
extern RTC_DATA_ATTR float LastBattVolts;
extern float BattVolts;
extern int BattPercent;

void BatterySetup (void);
void StartBatteryTask ();
//void BatteryLoop();

#endif
