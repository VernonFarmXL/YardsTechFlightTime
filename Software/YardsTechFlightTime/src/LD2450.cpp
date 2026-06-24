/*
 *	An Arduino library for the Hi-Link LD2450 24Ghz FMCW radar sensor.
 *
 *  This sensor is a Frequency Modulated Continuous Wave radar, which makes it good for presence detection and its sensitivity at different ranges to both static and moving targets can be configured.
 *
 *	The code in this library is based off the https://github.com/0ingchun/arduino-lib_HLK-LD2450_Radar.
 *
 *	https://github.com/ncmreynolds/ld2410
 *
 *
 */
#ifndef LD2450_cpp
#define LD2450_cpp

#include "LD2450.h"

LD2450::LD2450() // Constructor function
{
}

LD2450::~LD2450() // Destructor function
{
}


void LD2450::begin(Stream &radarStream)
{
    LD2450::radar_uart = &radarStream;
    LD2450::last_target_data = "";
}

void LD2450::begin(HardwareSerial &radarStream, bool already_initialized)
{
    if (!already_initialized)
    {
        radarStream.begin(LD2450_SERIAL_SPEED);
        radarStream.setTimeout(10);
    }

    LD2450::radar_uart = &radarStream;

    LD2450::last_target_data = "";

    radarStream.onReceiveError([this](hardwareSerial_error_t error) {
        LD2450::handleUartError(error);
    });
}

#ifdef ENABLE_SOFTWARESERIAL_SUPPORT
    void LD2450::begin(SoftwareSerial &radarStream, bool already_initialized)
    {
        if (!already_initialized)
        {
            radarStream.begin(LD2450_SERIAL_SPEED);
            radarStream.setTimeout(10);
        }
    
        LD2450::radar_uart = &radarStream;
    
        LD2450::last_target_data = "";
    }
#endif


void LD2450::setNumberOfTargets(uint16_t _numTargets)
{
    if (_numTargets > LD2450_MAX_SENSOR_TARGETS)
    {
        _numTargets = LD2450_MAX_SENSOR_TARGETS;
    }

    LD2450::numTargets = _numTargets;
}

String LD2450::getLastTargetMessage()
{
    return LD2450::last_target_data;
}


bool LD2450::waitForSensorMessage(bool wait_forever){

    uint8_t read_result = 0;
    for(long i = 0; i < LD2450_DEFAULT_RETRY_COUNT_FOR_WAIT_FOR_MSG; i++){
        read_result = LD2450::read();
        if(read_result >= 0){
            return true;
        }
        delay(1);
        
        //.... :)
        if(wait_forever){
            i = 0;
        }
    }
    return false;
}



int LD2450::read()
{
    if (LD2450::radar_uart == nullptr)
    {
        return -2;
    }

    if (LD2450::radar_uart->available())
    {

        byte rec_buf[LD2450_SERIAL_BUFFER + LD2450_FRAME_SIZE] = "";

        // Prepend any bytes left over from a frame that was split across the previous read
        if (LD2450::pending_len > 0)
        {
            memcpy(rec_buf, LD2450::pending_buf, LD2450::pending_len);
        }

        const int new_len = LD2450::radar_uart->readBytes(rec_buf + LD2450::pending_len, LD2450_SERIAL_BUFFER);
        const int len = LD2450::pending_len + new_len;
        LD2450::pending_len = 0;

        // IF WE GOT DATA PARSE THEM
        if (new_len > 0)
        {
          return LD2450::ProcessSerialDataIntoRadarData(rec_buf, len);
        }
    }
    return -1;
}

uint16_t LD2450::getSensorSupportedTargetCount(){
    if(LD2450::numTargets < LD2450_MAX_SENSOR_TARGETS){
        return LD2450::numTargets;
    }
    
    return LD2450_MAX_SENSOR_TARGETS;
}

LD2450::RadarTarget LD2450::getTarget(uint16_t _target_id){
    if (_target_id >= LD2450_MAX_SENSOR_TARGETS){
        LD2450::RadarTarget tmp;
        tmp.valid = false;
        return tmp;
    }
    return LD2450::radarTargets[_target_id];
}

void LD2450::pushTargetHistory(uint16_t _target_id, const RadarTarget_t &_target, unsigned long _time)
{
    std::lock_guard<std::mutex> guard(LD2450::targetHistoryMutex);

    LD2450::targetHistorySeq[_target_id]++;

    RadarTarget_t entry = _target;
    entry.timestamp = _time;
    entry.sequence = LD2450::targetHistorySeq[_target_id];

    uint16_t head = LD2450::targetHistoryHead[_target_id];
    LD2450::targetHistory[_target_id][head] = entry;
    LD2450::targetHistoryTimes[_target_id][head] = _time;

    LD2450::targetHistoryHead[_target_id] = (head + 1) % LD2450_TARGET_HISTORY_SIZE;
    if (LD2450::targetHistoryCount[_target_id] < LD2450_TARGET_HISTORY_SIZE)
    {
        LD2450::targetHistoryCount[_target_id]++;
    }
}

LD2450::RadarTarget LD2450::getTargetHistory(uint16_t _target_id, uint16_t _history_index)
{
    std::lock_guard<std::mutex> guard(LD2450::targetHistoryMutex);

    if (_target_id >= LD2450_MAX_SENSOR_TARGETS || _history_index >= LD2450::targetHistoryCount[_target_id])
    {
        LD2450::RadarTarget tmp;
        tmp.valid = false;
        return tmp;
    }

    // _history_index 0 = most recently written entry
    uint16_t head = LD2450::targetHistoryHead[_target_id];
    uint16_t idx = (head + LD2450_TARGET_HISTORY_SIZE - 1 - _history_index) % LD2450_TARGET_HISTORY_SIZE;
    return LD2450::targetHistory[_target_id][idx];
}

unsigned long LD2450::getTargetHistoryTime(uint16_t _target_id, uint16_t _history_index)
{
    std::lock_guard<std::mutex> guard(LD2450::targetHistoryMutex);

    if (_target_id >= LD2450_MAX_SENSOR_TARGETS || _history_index >= LD2450::targetHistoryCount[_target_id])
    {
        return 0;
    }

    uint16_t head = LD2450::targetHistoryHead[_target_id];
    uint16_t idx = (head + LD2450_TARGET_HISTORY_SIZE - 1 - _history_index) % LD2450_TARGET_HISTORY_SIZE;
    return LD2450::targetHistoryTimes[_target_id][idx];
}

uint16_t LD2450::getTargetHistoryCount(uint16_t _target_id)
{
    std::lock_guard<std::mutex> guard(LD2450::targetHistoryMutex);

    if (_target_id >= LD2450_MAX_SENSOR_TARGETS)
    {
        return 0;
    }
    return LD2450::targetHistoryCount[_target_id];
}

uint32_t LD2450::getTargetHistorySequence(uint16_t _target_id)
{
    std::lock_guard<std::mutex> guard(LD2450::targetHistoryMutex);

    if (_target_id >= LD2450_MAX_SENSOR_TARGETS)
    {
        return 0;
    }
    return LD2450::targetHistorySeq[_target_id];
}

void LD2450::handleUartError(hardwareSerial_error_t error)
{
    if (error == UART_BUFFER_FULL_ERROR || error == UART_FIFO_OVF_ERROR)
    {
        LD2450::bufferOverflowed = true;
        LD2450::bufferOverflowCount = LD2450::bufferOverflowCount + 1;
    }
}

bool LD2450::hasBufferOverflowed()
{
    bool overflowed = LD2450::bufferOverflowed;
    LD2450::bufferOverflowed = false;
    return overflowed;
}

uint32_t LD2450::getBufferOverflowCount()
{
    return LD2450::bufferOverflowCount;
}

int LD2450::ProcessSerialDataIntoRadarData(byte rec_buf[], int len)
{
  //unsigned long startTime = millis();
    int redreshed_targets = 0;

    int i = 0;
    for (; i < len; i++)
    {
        if (rec_buf[i] != 0xAA)
        {
            continue;
        }

        // Not enough bytes yet for a full frame - keep them and wait for the rest on the next read
        if (i + LD2450_FRAME_SIZE - 1 >= len)
        {
            break;
        }

        // Checking the header and footer
        if (rec_buf[i + 1] == 0xFF && rec_buf[i + 2] == 0x03 && rec_buf[i + 3] == 0x00 && rec_buf[i + 28] == 0x55 && rec_buf[i + 29] == 0xCC)
        {

            int index = i + 4; // Skip header and in-frame data length fields
            LD2450::last_target_data = "";

            for (uint16_t targetCounter = 0; targetCounter < LD2450_MAX_SENSOR_TARGETS; targetCounter++)
            {

                
                
                if (index + 7 < len)
                {
                    LD2450::RadarTarget target;
                    target.x = (int16_t)(rec_buf[index] | (rec_buf[index + 1] << 8));
                    target.y = (int16_t)(rec_buf[index + 2] | (rec_buf[index + 3] << 8));
                    target.speed = (int16_t)(rec_buf[index + 4] | (rec_buf[index + 5] << 8));
                    target.resolution = (uint16_t)(rec_buf[index + 6] | (rec_buf[index + 7] << 8));

                    // debug_serial.println(target.x);
                    // debug_serial.println(target.y);
                    // debug_serial.println(target.speed);

                    // Check the highest bit of x and y. Adjust the sign
                    if (rec_buf[index + 1] & 0x80)
                        target.x -= 0x8000;
                    else
                        target.x = -target.x;
                    if (rec_buf[index + 3] & 0x80)
                        target.y -= 0x8000;
                    else
                        target.y = -target.y;
                    if (rec_buf[index + 5] & 0x80)
                        target.speed -= 0x8000;
                    else
                        target.speed = -target.speed;


                    //CALCULATE DISTANCE
                    target.distance = sqrt(pow(target.x, 2) +  pow(target.y, 2));

                    // IF A RESOLUTION IS PRESENT THEN WE CAN ASSUME THAT A TARGET WAS FOUND
                    if(target.resolution != 0){
                        target.valid = true;
                    }else{
                       target.valid = false;
                    }

                    LD2450::radarTargets[targetCounter].id = targetCounter + 1;
                    LD2450::radarTargets[targetCounter].x = target.x;
                    LD2450::radarTargets[targetCounter].y = target.y;
                    LD2450::radarTargets[targetCounter].speed = target.speed;
                    LD2450::radarTargets[targetCounter].resolution = target.resolution;
                    LD2450::radarTargets[targetCounter].valid = target.valid;
                    LD2450::radarTargets[targetCounter].distance = target.distance;

                    LD2450::pushTargetHistory(targetCounter, LD2450::radarTargets[targetCounter], millis());


                    // Add target information to the string
                    LD2450::last_target_data += "TARGET ID=" + String(targetCounter + 1) + " X=" + String(target.x) + "mm, Y=" + String(target.y) + "mm, SPEED=" + String(target.speed) + "cm/s, RESOLUTION=" + String(target.resolution) + "mm, DISTANCE=" + String(target.distance) + "mm, VALID=" + String(target.valid) + "\n";

                    index += 8; // Move to the start of the next target data

                    redreshed_targets++;

                    //SKIP IF USER ONLY REQUESTED X VALID TARGETS
                    if(redreshed_targets >= LD2450::numTargets){
                        break;
                    }
                }else{
                    LD2450::radarTargets[targetCounter].valid = false;
                }
                


            }
            i = index; // Updating the index of an external loop
        }
    }
    // Stash any unconsumed bytes (start of a frame that hasn't fully arrived yet) for the next read
    if (i < len)
    {
        LD2450::pending_len = len - i;
        memcpy(LD2450::pending_buf, rec_buf + i, LD2450::pending_len);
    }

    //log_fxl ("%d", millis()-startTime);
    return redreshed_targets;
}

#endif