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
#ifndef LD2450_h
#define LD2450_h




#include <Arduino.h>
#include <mutex>

#ifdef SoftwareSerial_h
    #define ENABLE_SOFTWARESERIAL_SUPPORT
#endif


#ifdef ENABLE_SOFTWARESERIAL_SUPPORT
    #include <SoftwareSerial.h>
#endif

#define LD2450_MAX_SENSOR_TARGETS 3
#define LD2450_SERIAL_BUFFER 512
#define LD2450_SERIAL_SPEED 256000
#define LD2450_DEFAULT_RETRY_COUNT_FOR_WAIT_FOR_MSG 1000
#define LD2450_FRAME_SIZE 30 // 4 byte header + 3*8 byte targets + 2 byte footer
#define LD2450_TARGET_HISTORY_SIZE 200 // number of past readings kept per target





class LD2450
{

public:
    typedef struct RadarTarget
    {
        uint16_t id;         // ID
        int16_t x;           // X mm
        int16_t y;           // Y mm
        int16_t speed;       // cm/s
        uint16_t resolution; // mm
        uint16_t distance; // mm calculated from  x y
        bool valid;
        unsigned long timestamp = 0; // millis() when added to the history circular buffer
        uint32_t sequence = 0;       // this target's position in the uncapped per-target push sequence
    } RadarTarget_t;

    LD2450();
    // Constructor function
    ~LD2450();

    void begin(Stream &radarStream);
    void begin(HardwareSerial &radarStream, bool already_initialized = false);

#ifdef ENABLE_SOFTWARESERIAL_SUPPORT
    void begin(SoftwareSerial &radarStream, bool already_initialized = false);
#endif

    bool waitForSensorMessage(bool wait_forever = false);
    void setNumberOfTargets(uint16_t _numTargets);
    int ProcessSerialDataIntoRadarData(byte rec_buf[], int len);
    RadarTarget getTarget(uint16_t _target_id);
    uint16_t getSensorSupportedTargetCount();
    String getLastTargetMessage();
    int read();

    // History: index 0 is the most recent reading, up to getTargetHistoryCount()-1 (max LD2450_TARGET_HISTORY_SIZE)
    RadarTarget getTargetHistory(uint16_t _target_id, uint16_t _history_index);
    uint16_t getTargetHistoryCount(uint16_t _target_id);
    unsigned long getTargetHistoryTime(uint16_t _target_id, uint16_t _history_index);
    // Monotonically increasing count of readings ever pushed for a target (never resets/caps), used by
    // consumers to detect how many new entries have arrived since they last checked
    uint32_t getTargetHistorySequence(uint16_t _target_id);

    // True if the UART rx buffer/FIFO has overflowed (data lost) since the last call - only reported when
    // begin(HardwareSerial&, ...) is used, since this relies on HardwareSerial::onReceiveError()
    bool hasBufferOverflowed();
    uint32_t getBufferOverflowCount(); // total overflow events seen since begin()

protected:

private:
    Stream *radar_uart = nullptr;
    RadarTarget_t radarTargets[LD2450_MAX_SENSOR_TARGETS]; // Stores the target of the current frame
    uint16_t numTargets = LD2450_MAX_SENSOR_TARGETS;
    String last_target_data = "";
    byte pending_buf[LD2450_FRAME_SIZE]; // bytes left over from a frame split across two reads
    uint16_t pending_len = 0;

    // Circular buffer of the last LD2450_TARGET_HISTORY_SIZE readings per target slot
    RadarTarget_t targetHistory[LD2450_MAX_SENSOR_TARGETS][LD2450_TARGET_HISTORY_SIZE];
    unsigned long targetHistoryTimes[LD2450_MAX_SENSOR_TARGETS][LD2450_TARGET_HISTORY_SIZE];
    uint16_t targetHistoryHead[LD2450_MAX_SENSOR_TARGETS] = {0}; // next slot to write to
    uint16_t targetHistoryCount[LD2450_MAX_SENSOR_TARGETS] = {0}; // entries filled so far, capped at LD2450_TARGET_HISTORY_SIZE
    uint32_t targetHistorySeq[LD2450_MAX_SENSOR_TARGETS] = {0}; // total entries ever pushed, uncapped
    std::mutex targetHistoryMutex;

    void pushTargetHistory(uint16_t _target_id, const RadarTarget_t &_target, unsigned long _time);

    volatile bool bufferOverflowed = false;
    volatile uint32_t bufferOverflowCount = 0;
    void handleUartError(hardwareSerial_error_t error);
};
#endif