#ifndef _YTCOMMANDS_H
#define _YTCOMMANDS_H

extern unsigned long LastEIDTime;
extern bool TagReadFlag;


void CommandsSetup (void);
void StartCommandsTask ();

bool TagCaptured (volatile byte RawData[], int TagType);
void SetWifiCommand (int ClientNo, const char* Cmd);
void SetWifiResponse(int ClientNo, const char* Cmd);
String GetWifiResponse(int ClientNo);
void SetWifiAllClientResponse(int ClientNo, const char* Cmd);
String GetWifiAllClientResponse(int ClientNo);

#endif
