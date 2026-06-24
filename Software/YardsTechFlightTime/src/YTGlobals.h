#ifndef _YTGLOBALS_H
#define _YTGLOBALS_H

#define MAIN_DEBUG
//#define BATT_DEBUG
//#define BUZZ_DEBUG 
//#define CMD_DEBUG
//#define HDX_DEBUG
//#define BT_DEBUG
//#define TAG_FINDER_DEBUG 
//#define TRANSMIT_DEBUG 
//#define WIFI_DEBUG


#define LOCAL_BT_IMPLMENTATION //using our versions of the BluetoothSerial files

#include "arduino.h"
#include <Preferences.h>   // used for settings
#include "BluetoothSerial.h"

#ifdef log_fxl
#undef log_fxl
#endif
#define log_fxl(format, ...) Serial.printf("[I][%s:%u] %s(): " format "\r\n", \
    __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)



//this defines what hardware we are compiling for
#define HARDWARE_VERSION 200 //initial version
#define FIRMWARE_VERSION "2025.11.15"

//loop delays and priorities
#define MAIN_LOOP_DELAY 8 //5 //10
#define MAIN_LOOP_PRIORITY 2 //2 //5

#define WIFI_LOOP_DELAY 1 //5 //10 //8 //5 //10 //20
#define WIFI_LOOP_PRIORITY 1

#define BTMAST_LOOP_DELAY 15 //12 //1
#define BTMAST_LOOP_PRIORITY 2

#define BTCLIENT_LOOP_DELAY 25 //12 //1
#define BTCLIENT_LOOP_PRIORITY 2

//io pins
#define TRANSMIT_ENABLE_OUTPUT 2 //goes high when transmit is enabled
#define HDX_DATA_IN 4 //Input from HDX receiver
#define BUZZER_PIN 5 //not used
#define POWER_ON_OUTPUT 12 //turn on the mosfet to connect battery
const gpio_num_t BATT_CHARGED_PIN =  GPIO_NUM_13; //led to show that battery is charged
#define EXT_POWER_PIN 14 //led to show that external power is connected
#define NC_3 15 //not used
//#define TRANS_PWR_EN 16 //pin used to turn on the transmit voltage regulator
//#define NC_2 17 //PWM pin for buzzer
#define NC_4 18 //not used
#define TUNE_1 19 //Tune pin 1
#define TUNE_2 21 //Tune pin 2
#define ACTIVITY_PIN 22// used to display activity led
#define COMMS_PIN 23// used to display comms led
#define PWM_TRANSMIT_PIN 25 //transmitter for RF using PWM
#define TUNE_3 26 //tune pin 3
#define TUNE_4 27 //tune pin 4
#define TUNE_5 32 //tune pin 5
#define NOISE 33 //not used
#define EXT_POWER_INPUT 34 //high on this pin indicates that external power connected
#define POWER_BTN 35 //monitors the power button
#define ANT_VOLTS 36 //analogue input for antenna voltage
#define BATT_VOLTS 39 //analogue input for battery monitor

#define PROT_STD  0 //standard protocol
#define PROT_MQTT 1 //MQTT protocol

#define CONN_WAIT_YT_THEN_AP  0 //wifi connect to YardsTech network and if can't locate then start as an access point
#define CONN_YT               1 //wifi connect to YardsTech network
#define CONN_ACCESS_POINT     2 //wifi setup as an access point
#define CONN_EXT_NETWORK      3 //wifi connecting to an external wifi network
#define CONN_BT_CLIENT        4 //standard bluetooth client
#define CONN_BT_MASTER        5 //standard bluetooth master

//#define BT_CLIENT      0 //standard bluetooth client
//#define BT_READ_MASTER 1 //BT reader connected as master to this unit
//#define BT_MASTER      2 //standard BT as master
//#define BT_READ_SLAVE  3 //BT reader connected as slave to this unit
//#define BT_DISABLED    4 //BT not used

#define CURRENT_DATA_MOD 2 //current eeprom data version

#define DEF_BT_NAME "YTPanelBT" 
#define DEF_SSID "YardsTech"
#define DEF_YTIP "192.168.4.1"

#define DEF_STATIC_IP "192.168.4.40"
#define DEF_STATIC_GATEWAY "192.168.4.1"
#define DEF_STATIC_SUBNET "255.255.255.0"


#define MAX_CLIENTS 10
#define SSID_LEN 26 //length of the ssid char array
#define PASSWORD_LEN 26 //length of the password char array

#define HDX_TAG_TYPE 1

#define TOTAL_CLIENTS MAX_CLIENTS + 2 //the + 1 for a bluetooth client, +1 for YT client;
#define SCALE_CLIENT_IDX MAX_CLIENTS     //the index of the scale client. 
#define BT_CLIENT_IDX MAX_CLIENTS + 1 //the index of the bt client. It's at the end of the array;
#define MAX_EID_TIME 5000
#define MAX_LINE_LENGTH 50

//settings names
#define GUIDEEAddr "GUID"
#define SSIDEEAddr "SSID" 
#define PasswordEEAddr "Password"
#define SSID2EEAddr "SSID2"
#define Password2EEAddr "Password2" 
#define SSID3EEAddr "SSID3" 
#define Password3EEAddr "Password3"
#define ServerEEAddr "Server" 
#define ConnectionTypeEEAddr "ConType"
#define ProtocolTypeEEAddr "ProtType"
#define BTClientEnabledEEAddr "BTCliEn"
#define DataModNoEEAddr "DataModNo"
#define BattVoltOffsetEEAddr "BattOffset"
//#define BTTypeEEAddr "BTType"
#define SerialNoEEAddr "SerialNo"
#define BTPinEEAddr "BTPin"
#define BTDeviceNameEEAddr "BTDeviceName"
#define BTDeviceAddrEEAddr "BTDeviceAddr"
#define WifiNameEEAddr "WifiName"
#define BTNameEEAddr "BTName" 
#define DefaultSSIDEEAddr "DefaultSSID"
#define DefaultYTIPEEAddr "DefaultYTIP"
#define DefaultPasswordEEAddr "DefaultPassword"
#define ContReadEEAddr "ContRead"
#define QuietEEAddr "Quiet"
#define NoVibrEEAddr "NoVibr"
#define HDXLongChargeTimeEEAddr "HDXLongCharge"
#define HDXShortChargeTimeEEAddr "HDXShortCharge"
//#define HDXWaitTimeEEAddr "HDXWaitTime"
//#define HDXWaitTimeContEEAddr "HDXWaitTimeCont"
#define Ignore7and8 "Ignore7and8"
#define ucMCW1EEAddr "ucMCW1"
#define BuzzFreqEEAddr "BuzzFreq"
#define OneTagReadEEAddr "OneTagRead"
#define BtSSPModeEnabledEEAddr "BtSSP"
#define BtEIDOutputFormatEEAddr "BtFormat"
#define ZPLCodeEEAddr "ZPLCode"
#define DefaultStaticIPEEAddr "StatIP"
#define DefaultStaticGatewayEEAddr "StatGate"
#define DefaultStaticSubnetEEAddr "StatSub"
#define UseStaticIPEEAddr "UseStatic"
#define TransmitDutyEEAddr "TransDuty"
#define AutoTuneEEAddr "AutoTune"
//#define AutoTuneTxValueEEAddr "AutoTuneTx"
//#define AutoTuneRxValueEEAddr "AutoTuneRx"
#define RxTuneOffsetEEAddr "RxTuneOff"
#define PowerSaveEEAddr "PwrSave"
#define NoiseOffsetEEAddr "NoiseOff"
#define SafeReadEEAddr "SafeRead"


#define MAX_BUFFER_LENGTH 50

#define MAX_WAIT_TIME 30000 //the maximum wait time/retry time is 30 seconds
#define MIN_WAIT_TIME 5000 //the minimum wait time/retry time is 5 seconds. Must be longer than INQ_LEN time
#define WAIT_TIME_INC 2000 //the amount of time to add to the current wait time after each sucessive tries ... up to MAX_WAIT_TIME

enum Priority {
    LOW_PRI = tskIDLE_PRIORITY + 1,
    NORMAL_PRI = tskIDLE_PRIORITY + 2,
    HIGH_PRI = tskIDLE_PRIORITY + 3,
    HIGHEST_PRI = tskIDLE_PRIORITY + 4
};



extern BluetoothSerial SerialBT;

extern Preferences Settings;

extern bool BTClientConnected;
extern bool LastBTClientConnected;
extern String BTAddress;
extern String BTPin;
extern String BTDeviceName;
extern String BTDeviceAddr;
extern bool ForceBTOn;
extern String BTName;
extern bool BtSSPModeEnabled;
extern int BtEIDOutputFormat;
extern String ZPLCode;
extern char BtInput[MAX_BUFFER_LENGTH];
extern bool BtDisabled;
extern unsigned int LastConnectTime; //the last time a BT connection was tried
extern unsigned int CurrentWaitTime; //time to wait before reconnecting BT

// extern bool ContinuousReads;
// extern bool _ContinuousReads;
// extern unsigned int ReadCount;

extern bool ContinuousTransmit;
extern bool TransmitEnabled;

extern bool SetupMode;
extern bool SafeMode;
extern bool PoweringDown;
extern bool OTAUpdating;

extern int ConnectionType; //0 = wifi server AP, 1 = wifi server, 2 = wifi client, 3 = BT, 4 = BLE
extern int CurrentConnectionType;
extern int ProtocolType; //0 = Standard, 1 = MQTT 
extern String GUIDStr;

extern String DefaultSSID;//"mavn-wireless4";
extern String DefaultPassword;//"qwertyuiopasd";
extern String DefaultScaleIP;
extern String DefaultStaticIP;;
extern String DefaultStaticGateway;
extern String DefaultStaticSubnet;
extern bool UseStaticIP;

extern String ssid;//"mavn-wireless4";
extern String Password;//"qwertyuiopasd";
extern String ssid2;
extern String Password2;
extern String ssid3;
extern String Password3;
extern int WifiNum;

extern String WifiName;

extern volatile unsigned int AvgCount;

extern String LastEID;
extern int LastTagType;
//extern bool IgnoreBytes7and8;

extern String SerialNo;
extern int DataModNo;

extern bool ExternalPoweredOn;
extern bool UserPoweredOn;

extern bool ActivityLED;

extern unsigned long LastReadStopped;



bool ValidCRC(volatile byte RawData[]);
void printDeviceAddress();
void FactoryReset (int ModNo);
void GoToSleep ();
void RestartModule(bool Forced);
String uint64ToString(uint64_t input);

void StopRead (unsigned long Now);


#endif













 
