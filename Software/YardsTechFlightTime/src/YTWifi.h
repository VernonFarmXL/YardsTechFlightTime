#ifndef _YTWIFI_H
#define _YTWIFI_H

extern int CommsPin;
//extern int ClientNo;
extern bool AwaitingWeightResponse;


void WifiSetup();
void StartWifiLoopTask ();
//void WifiLoop ();
void StartStopScanForYT ();

#endif
