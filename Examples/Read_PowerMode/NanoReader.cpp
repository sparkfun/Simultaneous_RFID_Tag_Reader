

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "NanoReader.h"

RFID::RFID(void)
{
  // Constructor
}

//Initialize the Serial port
bool RFID::begin(Stream &serialPort)
{
  _nanoSerial = &serialPort; //Grab which port the user wants us to use

  //_nanoSerial->begin(); //Stream has no .begin() so the user has to do a whateverSerial.begin(xxxx); from setup()
}

//Set baud rate
//Takes in a baud rate
//Returns response in the msg array
void RFID::cmdSetBaud(uint8_t *msg, long baudRate)
{
  byte i = 2;
  SETU8(msg, i, TMR_SR_OPCODE_SET_BAUD_RATE); //Load this opcode into msg array
  SETU32(msg, i, baudRate); //Load the 4 byte variable into the array
  msg[1] = i - 3; //Load the length of this operation into msg array. This one is 4 bytes

  sendCommand(msg, false); //Do not wait for a response
}

//Get the version number from the module
void RFID::cmdGetVersion(uint8_t *msg)
{
  uint8_t i = 2;
  SETU8(msg, i, TMR_SR_OPCODE_VERSION); //Stores opcode into MSG array at spot 2, increments i
  msg[1] = i - 3; //Install length, length of data with this opcode = 0

  sendCommand(msg); //Wake module, surround message, send it with a known timeout, default timeout is 1000(ms?)
  //sendCommand passes msg by reference
  //msg now contains the response

  //return response;
}

//Given an array, calc CRC, assign header, sent it out
//Returns a success message in uint form
void RFID::sendCommand(uint8_t *msg, boolean waitForResponse)
{
  //Wake module, surround message, send it with a known timeout, default timeout is 1000(ms?)

  msg[0] = 0xFF;
  byte messageLength = msg[1];
  byte opcode = msg[2]; //Used to see if response from module has the same opcode

  //Attach CRC
  uint16_t crc = calculateCRC(&msg[1], messageLength + 2); //Calc CRC starting from spot 1, not 0. Add 2 for LEN and OPCODE bytes.
  msg[messageLength + 3] = crc >> 8;
  msg[messageLength + 4] = crc & 0xff;

  //Remove anything in the incoming buffer
  //TODO this is a bad idea if we are constantly readings tags
  while(_nanoSerial->available()) _nanoSerial->read();
  
  //Send the command to the module
  for (byte x = 0 ; x < messageLength + 5 ; x++)
    _nanoSerial->write(msg[x]);

  //There are some commands (setBaud) that we can't or don't want the response
  if (waitForResponse == false) return;

  //For debugging, probably remove
  for(byte x = 0 ; x < 200 ; x++) msg[x] = 0;

  //Wait for response with timeout
  long startTime = millis();
  while (_nanoSerial->available() == false)
  {
    if (millis() - startTime > COMMAND_TIME_OUT)
    {
      Serial.println("Time out!");
      msg[0] = ERROR_COMMAND_RESPONSE_TIMEOUT;
      return;
    }
    delay(1);
  }

  // Layout of response in data array:
  // [0] [1] [2] [3]      [4]      [5] [6]  ... [LEN+4] [LEN+5] [LEN+6]
  // FF  LEN OP  STATUSHI STATUSLO xx  xx   ... xx      CRCHI   CRCLO
  messageLength = TMR_SR_MAX_PACKET_SIZE - 1;
  byte spot = 0;
  while(spot < messageLength)
  {
    if (millis() - startTime > COMMAND_TIME_OUT)
    {
      Serial.println("Time out!");
      msg[0] = ERROR_COMMAND_RESPONSE_TIMEOUT;
      return;
    }

    if (_nanoSerial->available())
    {
      Serial.print(".");
      
      msg[spot] = _nanoSerial->read();

      if (spot == 1) //Grab the length of this response (spot 1)
        messageLength = msg[1] + 7; //Actual length of response is ? + 7 for extra stuff (header, Length, opcode, 2 status bytes, ..., 2 bytes CRC = 7)

      spot++;
    }
  }

  //Check CRC
  crc = calculateCRC(&msg[1], messageLength - 3); //Remove header, remove 2 ctc bytes
  if ((msg[messageLength - 2] != (crc >> 8)) || (msg[messageLength - 1] != (crc & 0xff)))
  {
    msg[0] = ERROR_CORRUPT_RESPONSE;
    Serial.println("Corrupt response");
    return;
  }

  //If crc is ok, check that opcode matches (did we get a response to the command we asked or a different one?)
  if (msg[2] != opcode)
  {
    msg[0] = ERROR_WRONG_OPCODE_RESPONSE;
    Serial.println("Wrong opcode response");
    return;
  }

  //If everything is ok, load all ok into msg array
  msg[0] = ALL_GOOD;
}


/* Comes from serial_reader_l3.c
   ThingMagic-mutated CRC used for messages.
   Notably, not a CCITT CRC-16, though it looks close.
*/
static uint16_t crctable[] =
{
  0x0000, 0x1021, 0x2042, 0x3063,
  0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b,
  0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

//Calculates the magical CRC value
uint16_t RFID::calculateCRC(uint8_t *u8Buf, uint8_t len)
{
  uint16_t crc = 0xffff;

  for (byte i = 0 ; i < len ; i++)
  {
    crc = ((crc << 4) | (u8Buf[i] >> 4))  ^ crctable[crc >> 12];
    crc = ((crc << 4) | (u8Buf[i] & 0xf)) ^ crctable[crc >> 12];
  }

  return crc;
}
