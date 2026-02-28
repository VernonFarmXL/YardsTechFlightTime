#include "driver/rtc_io.h"
#include "esp_bt_device.h"
#include "BluetoothSerial.h"
#include "YTGlobals.h"
#include <Preferences.h>   // used for settings


BluetoothSerial SerialBT;



int ConnectionType = CONN_WAIT_YT_THEN_AP; //0 = wifi server AP, 1 = wifi server, 2 = wifi client, 3 = BT, 4 = BLE
int CurrentConnectionType = CONN_WAIT_YT_THEN_AP;
int ProtocolType = PROT_STD; //0 = Standard, 1 = MQTT 
String GUIDStr = "";

String DefaultSSID = DEF_SSID;//"mavn-wireless4";
String DefaultPassword = "";//"qwertyuiopasd";
String DefaultScaleIP = DEF_YTIP;
String DefaultStaticIP = DEF_STATIC_IP;
String DefaultStaticGateway = DEF_STATIC_GATEWAY;
String DefaultStaticSubnet = DEF_STATIC_SUBNET;
bool UseStaticIP = true;
String ssid = "";//"mavn-wireless4";
String Password = "";//"qwertyuiopasd";
String ssid2 = "";
String Password2 = "";
String ssid3 = "";
String Password3 = "";
int WifiNum = 0;

String WifiName = "";
String BTName = "";

char Server[16] = "";//"192.168.10.23";

bool SetupMode = false;
bool SafeMode = false;
bool ActivityLED = LOW;
bool PoweringDown = false;
bool OTAUpdating = false;

volatile unsigned int AvgCount = 0;

//int BTType = BT_CLIENT;
bool BTClientConnected = false;
bool LastBTClientConnected = false;
String BTAddress = "";
String BTPin = "";
String BTDeviceName = "";
String BTDeviceAddr = "";
bool BtSSPModeEnabled = false;
int BtEIDOutputFormat = 0; //0=plain text, 1=ZPL
String ZPLCode = "";
bool ForceBTOn = false;
unsigned int LastConnectTime; //the last time a BT connection was tried
unsigned int CurrentWaitTime = MIN_WAIT_TIME; //time to wait before reconnecting BT
String EIDRead = "";
char BtInput[MAX_BUFFER_LENGTH] = { 0 };
bool BtDisabled = false;

String LastEID = "";
int LastTagType = HDX_TAG_TYPE;
//bool IgnoreBytes7and8 = true;

String SerialNo = "";
int DataModNo;

bool ExternalPoweredOn = false;

bool UserPoweredOn = false;

bool ContinuousTransmit = false;
bool TransmitEnabled = true;

// bool ContinuousReads = false;
// bool _ContinuousReads = false;
// unsigned int ReadCount = 0;


Preferences Settings;




void RestartModule(bool Forced){
  log_i("Restarting module");
  digitalWrite (COMMS_PIN, LOW); 
  digitalWrite (ACTIVITY_PIN, LOW); 
  digitalWrite (BATT_CHARGED_PIN, LOW); 
  digitalWrite (EXT_POWER_PIN, LOW);
  delay (250); //leave time for serial comms to empty buffer
  ESP.restart();
}


void GoToSleep () {
  UserPoweredOn = false;
  log_d("Preparing to sleep");
  if (digitalRead (EXT_POWER_INPUT) == HIGH){
    log_i("Externally powered, so will stay running");
    log_i("Resetting to turn off any services");
    RestartModule (false);
  }
  else {
    log_i("Shutting down power");
    digitalWrite (COMMS_PIN, LOW); 
    digitalWrite (ACTIVITY_PIN, LOW); 
    digitalWrite (BATT_CHARGED_PIN, LOW); 
    digitalWrite (EXT_POWER_PIN, LOW);
    //digitalWrite (EXT_POWER_PIN, LOW);
    delay(1000); //alow time for any serial comms to be sent 
    digitalWrite (POWER_ON_OUTPUT, LOW); //turn off power
  }
}


void FactoryReset (int ModNo) {
  Settings.begin("Settings", false);
  if (ModNo > CURRENT_DATA_MOD) {
    ModNo = 0;
  }
  
  if (ModNo <= 0){ //this means the data has never been set i.e. initial startup
    log_i ("Setting data mod no 1");
    Settings.putString(SSIDEEAddr, "");  
    Settings.putString(PasswordEEAddr, "");  
    Settings.putString(ServerEEAddr, "");  
    Settings.putInt(ConnectionTypeEEAddr, CONN_WAIT_YT_THEN_AP);  
    Settings.putInt(ProtocolTypeEEAddr, PROT_STD);  
    Settings.putBool(BTClientEnabledEEAddr, true);  
    Settings.putString(SSID2EEAddr, "");  
    Settings.putString(Password2EEAddr, "");  
    Settings.putString(SSID3EEAddr, "");  
    Settings.putString(Password3EEAddr, "");  
    //Settings.putInt(BTTypeEEAddr, BT_CLIENT);  
    Settings.putString(BTPinEEAddr, "");  
    Settings.putString(BTDeviceNameEEAddr, "");  
    Settings.putString(BTDeviceAddrEEAddr, "");  
    Settings.putString(WifiNameEEAddr, DEF_SSID);  
    Settings.putString(BTNameEEAddr, DEF_BT_NAME);  
    Settings.putFloat(BattVoltOffsetEEAddr, 0);  
    Settings.putString(DefaultSSIDEEAddr, String(DEF_SSID));  
    Settings.putString(DefaultYTIPEEAddr, String(DEF_YTIP));  
    Settings.putString(DefaultPasswordEEAddr, "");  
    Settings.putBool(ContReadEEAddr, false);  
    Settings.putBool(QuietEEAddr, false);  
    Settings.putBool(NoVibrEEAddr, false);  
    Settings.getBool(BtSSPModeEnabledEEAddr, false);
    Settings.getInt(BtEIDOutputFormatEEAddr, 0); //default to plain text
    Settings.getString(ZPLCodeEEAddr, "");
    Settings.putString(DefaultStaticIPEEAddr, String(DEF_STATIC_IP));  
    Settings.putString(DefaultStaticGatewayEEAddr, String(DEF_STATIC_GATEWAY));  
    Settings.putString(DefaultStaticSubnetEEAddr, String(DEF_STATIC_SUBNET));  
    Settings.putBool(UseStaticIPEEAddr, true); 
 }
 
  DataModNo = CURRENT_DATA_MOD;
  Settings.putInt(DataModNoEEAddr, DataModNo);//set current data mod

  Settings.end ();
}


void printDeviceAddress() {
  const uint8_t* point = esp_bt_dev_get_address();
 
  for (int i = 0; i < 6; i++) {
 
    char str[3];
 
    sprintf(str, "%02X", (int)point[i]);
    BTAddress = BTAddress + str;
 
    if (i < 5){
      BTAddress = BTAddress + ":";
    }
  }
}


String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c +='0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);

  while (result.length() < 12) {
    result = "0" + result;
  }
  
  return result;
}



/**********************************************************************
    *  There are two ways to generate Cyclic Redundancy Check (CRC) code:
    *  the lookup table method and the loop method. Both methods generate
    *  identical CRC code. The difference is that lookup table generate CRC
    *  code 2-3 times faster than loop method, with the cost of 500 ~ 600
    *  byte extra code size. Users are free to choose either of the method
    *  to generate CRC code, as long as it fits the particular application
    *  needs.
    ***********************************************************************/
const unsigned int CRC16Table[256] =
 {
    0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF, 0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
    0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E, 0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
    0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD, 0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
    0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C, 0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
    0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB, 0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
    0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A, 0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
    0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9, 0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
    0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738, 0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
    0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7, 0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
    0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036, 0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
    0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5, 0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
    0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134, 0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
    0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3, 0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
    0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232, 0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
    0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1, 0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
    0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330, 0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
  };
   
bool ValidCRC(volatile byte RawData[]) {
  unsigned int crc, tmp;
  int i;
  unsigned long ZeroCheck = 0; //add up att the bytes and make sure they are not all 0. All 0's gives a valid CRC
       
  crc = 0x00; //Init crc to 0
       
  for(i=0;i<8;i++) {// message is 8 bytes long
    ZeroCheck = ZeroCheck + RawData[i];
    tmp=crc ^ ((unsigned int)RawData[i]);
    crc = (crc >> 8) ^ CRC16Table[tmp & 0x00FF];
  }
  
  return  (crc == ((unsigned int)RawData[8] + ((unsigned int)RawData[9]<<8))) && (ZeroCheck != 0);
}
