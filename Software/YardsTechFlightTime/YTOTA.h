#ifndef _YTOTA_H
#define _YTOTA_H


extern bool OTAUpdating;
extern String OTAHost;
extern int OTAPort;
extern String OTAPath;


void ExecuteOTA (String Host, int Port, String Path);

#endif
