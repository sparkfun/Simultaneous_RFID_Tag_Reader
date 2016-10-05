/*
  This is an example of how to write a library that allows user to pass in a Serial port

  Nathan Seidle
  SparkFun Electronics

  License: Public domain
*/

#include "Arduino.h" //Needed for Stream


#define TMR_SR_MAX_PACKET_SIZE 256

#define TMR_SR_OPCODE_VERSION 0x03
#define TMR_SR_OPCODE_SET_BAUD_RATE 0x06
#define TMR_SR_OPCODE_READ_TAG_ID_SINGLE 0x21
#define TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE 0x22
#define TMR_SR_OPCODE_WRITE_TAG_ID 0x23
#define TMR_SR_OPCODE_KILL_TAG 0x26
#define TMR_SR_OPCODE_GET_POWER_MODE 0x68

#define COMMAND_TIME_OUT  1000 //Number of ms before stop waiting for response from module
#define ALL_GOOD 0
#define ERROR_COMMAND_RESPONSE_TIMEOUT  1
#define ERROR_CORRUPT_RESPONSE          2
#define ERROR_WRONG_OPCODE_RESPONSE     3

//This copies a u8value to a given spot in the provided array
//It then advances the array pointer 1 spot
#define SETU8(msg, i, u8val) do {      \
    (msg)[(i)++] = (u8val)      & 0xff;  \
  } while (0)

#define SETU32(msg, i, u32val) do {    \
    uint32_t _tmp = (u32val);            \
    (msg)[(i)++] = (uint8_t)(_tmp >> 24) & 0xff;  \
    (msg)[(i)++] = (uint8_t)(_tmp >> 16) & 0xff;  \
    (msg)[(i)++] = (uint8_t)(_tmp >>  8) & 0xff;  \
    (msg)[(i)++] = (uint8_t)(_tmp >>  0) & 0xff;  \
  } while (0)

#define GETU8AT(msg, i) ( \
                          ((msg)[(i)])          )

#define TMR_SUCCESS    0
#define TMR_ERROR_COMM 1
#define TMR_ERROR_CODE 2
#define TMR_ERROR_MISC 3
#define TMR_ERROR_LLRP 4

/**
   Defines the values for the parameter @c /reader/powerMode and the
   return value and parameter to TMR_SR_cmdGetPowerMode and
   TMR_SR_cmdSetPowerMode.
*/
typedef enum TMR_SR_PowerMode
{
  TMR_SR_POWER_MODE_MIN     = 0,
  TMR_SR_POWER_MODE_FULL    = 0,
  TMR_SR_POWER_MODE_MINSAVE = 1,
  TMR_SR_POWER_MODE_MEDSAVE = 2,
  TMR_SR_POWER_MODE_MAXSAVE = 3,
  TMR_SR_POWER_MODE_SLEEP   = 4,
  TMR_SR_POWER_MODE_MAX     = TMR_SR_POWER_MODE_SLEEP,
  TMR_SR_POWER_MODE_INVALID = TMR_SR_POWER_MODE_MAX + 1,
} TMR_SR_PowerMode;

class RFID
{
  public:
    RFID(void);

    bool begin(Stream &serialPort = Serial); //If user doesn't specificy then Serial will be used

    void cmdSetBaud(uint8_t *msg, long baudRate);
    void cmdGetVersion(uint8_t *msg);
    void sendCommand(uint8_t *msg, boolean waitForResponse = true);
    uint16_t calculateCRC(uint8_t *u8Buf, uint8_t len);

  private:

    Stream *_nanoSerial; //The generic connection to user's chosen serial hardware
};


