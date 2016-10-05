/*
  Reading multuple 920MHz RFID tags
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This is a stripped down implementation of the Mercury API from ThingMagic

  Module powers on at 115200bps by default. We can change baud rate to 9600bps but it is not saved to memory.

  Files to look at:
    serial_reader_l3.c
    serial_reader_imp.h
    tmr_utils.h

  Functions to create:
    readTagID (TMR_SR_OPCODE_READ_TAG_ID_SINGLE)
    readTagData
    writeTagID
    writeTagData
    setReadTXPower (TMR_SR_OPCODE_SET_READ_TX_POWER) / setWriteTXPower (or setPowerMode)
    setOperatingFreq (or setRegion)
    serBaudRate
    lockTag
    killTag

    startContiniousRead

*/

#include "NanoReader.h"
RFID nano; //Create instance

#include <SoftwareSerial.h> //Used for transmitting to the device

SoftwareSerial softSerial(3, 2); // RX, TX

uint8_t msg[TMR_SR_MAX_PACKET_SIZE]; //This is our universal msg array, used for all communication

void setup()
{
  Serial.begin(115200);
  Serial.println("Go");

  for (int x = 0 ; x < TMR_SR_MAX_PACKET_SIZE ; x++)
    msg[x] = 0;

  setupNano(9600); //Configure nano to run at 9600bps

  nano.cmdGetVersion(msg); //Get the version number from module

  delay(10);

  Serial.println("Response:");
  for (int x = 0 ; x < 10 ; x++)
  {
    Serial.print(" ");
    Serial.print(x);
    Serial.print("[");
    if(msg[x] < 0x10) Serial.print("0");
    Serial.print(msg[x], HEX);
    Serial.print("]");
  }
  Serial.println();

  //uint8_t currentMode;
  //cmdGetPowerMode(&currentMode);

  //Serial.print("mode: ");
  //Serial.println(currentMode);
}

void loop()
{

}

//Because Stream does not have a .begin() we have to do this outside the library
void setupNano(long baudRate)
{
  softSerial.begin(115200); //Start software serial at 115200, the defaul baud of the nano

  nano.begin(softSerial); //Give the library the software serial port

  nano.cmdSetBaud(msg, baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

  softSerial.end(); //Stop the software serial port
  
  delay(1);

  softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate

  delay(1);

  //We are now ready to communicate with the nano through the library
}

//TMR_Status TMR_SR_cmdGetPowerMode(TMR_Reader *reader, TMR_SR_PowerMode *mode)
/*uint8_t cmdGetPowerMode(uint8_t *mode)
{
  uint8_t response; //TMR_Status ret;
  uint8_t msg[TMR_SR_MAX_PACKET_SIZE];

  uint8_t i = 2;

  SETU8(msg, i, TMR_SR_OPCODE_GET_POWER_MODE); //Stores opcode into MSG array at spot 2, increments i

  msg[1] = i - 3; //Install length, length of data with this opcode = 0

  response = sendCommand(msg); //Wake module, surround message, send it with a known timeout, default timeout is 1000(ms?)
  //sendCommand passes msg by reference
  //msg now contains the response

  if (response != TMR_SUCCESS)
  {
    return response;
  }

  //Mode is what is stored in spot 5
  *mode = (TMR_SR_PowerMode)msg[5];//GETU8AT(msg, 5);

  return TMR_SUCCESS;
}*/

/*
  TMR_Status TMR_SR_msgSetupReadTagSingle(uint8_t *msg, uint8_t *i, TMR_TagProtocol protocol,TMR_TRD_MetadataFlag metadataFlags, const TMR_TagFilter *filter,uint16_t timeout)
  {
  uint8_t optbyte;

  SETU8(msg, *i, TMR_SR_OPCODE_READ_TAG_ID_SINGLE);
  SETU16(msg, *i, timeout);
  optbyte = *i;
  SETU8(msg, *i, 0); //Initialize option byte
  msg[optbyte] |= TMR_SR_GEN2_SINGULATION_OPTION_FLAG_METADATA;
  SETU16(msg,*i, metadataFlags);
  filterbytes(protocol, filter, &msg[optbyte], i, msg,0, true);
  msg[optbyte] |= TMR_SR_GEN2_SINGULATION_OPTION_FLAG_METADATA;
  return TMR_SUCCESS;

  }

  //Helper function to frame the multiple protocol search command
  TMR_Status
  TMR_SR_msgSetupMultipleProtocolSearch(TMR_Reader *reader, uint8_t *msg, TMR_SR_OpCode op, TMR_TagProtocolList *protocols, TMR_TRD_MetadataFlag metadataFlags, TMR_SR_SearchFlag antennas, TMR_TagFilter **filter, uint16_t timeout)
  {
  TMR_Status ret;
  uint8_t i;
  uint32_t j;
  uint16_t subTimeout;

  ret = TMR_SUCCESS;
  i=2;

  SETU8(msg, i, TMR_SR_OPCODE_MULTI_PROTOCOL_TAG_OP);  //Opcode
  if(reader->continuousReading)
  {
    //Timeout should be zero for true continuous reading
    SETU16(msg, i, 0);
    SETU8(msg, i, (uint8_t)0x1);//TM Option 1, for continuous reading
  }
  else
  {
    SETU16(msg, i, timeout); //command timeout
    SETU8(msg, i, (uint8_t)0x11);//TM Option, turns on metadata
    SETU16(msg, i, (uint16_t)metadataFlags);
  }

  SETU8(msg, i, (uint8_t)op);//sub command opcode

  if (TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)
  {
    reader->isStopNTags = false;
    //in case of multi read plan look for the stop N trigger option

    for (j = 0; j < reader->readParams.readPlan->u.multi.planCount; j++)
    {
      if (reader->readParams.readPlan->u.multi.plans[j]->u.simple.stopOnCount.stopNTriggerStatus)
      {
        reader->numberOfTagsToRead += reader->readParams.readPlan->u.multi.plans[j]->u.simple.stopOnCount.noOfTags;
        reader->isStopNTags = true;
      }
    }
  }
  else
  {
    reader->numberOfTagsToRead += reader->readParams.readPlan->u.simple.stopOnCount.noOfTags;
  }

  if (reader->isStopNTags && !reader->continuousReading)
  {
    //
    // True means atlest one sub read plan has the requested for stop N trigger.
    // Enable the flag and add the total tag count. This is only supported for sync read
    // case.
    //
    SETU16(msg, i, (uint16_t)TMR_SR_SEARCH_FLAG_RETURN_ON_N_TAGS);
    SETU32(msg, i, reader->numberOfTagsToRead);
  }
  else
  {
    SETU16(msg, i, (uint16_t)0x0000);//search flags, only 0x0001 is supported
  }

  //TODO:add the timeout as requested by the user
  subTimeout =(uint16_t)(timeout/(protocols->len));
  for (j=0;j<protocols->len;j++) // iterate through the protocol search list
  {
    int PLenIdx;


    TMR_TagProtocol subProtocol=protocols->list[j];
    SETU8(msg, i, (uint8_t)(subProtocol)); //protocol ID
    PLenIdx = i;
    SETU8(msg, i, 0); //PLEN

    //
    // in case of multi readplan and the total weight is not zero,
    // we should use the weight as provided by the user.
    //
    if (TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)
    {
      if (0 != reader->readParams.readPlan->u.multi.totalWeight)
      {
        subTimeout = (uint16_t)(reader->readParams.readPlan->u.multi.plans[j]->weight * timeout
          / reader->readParams.readPlan->u.multi.totalWeight);
      }
    }

    //In case of Multireadplan, check each simple read plan FastSearch option and
    // stop N trigger option.
    if (TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)
    {
      reader->fastSearch = reader->readParams.readPlan->u.multi.plans[j]->u.simple.useFastSearch;
      reader->triggerRead = reader->readParams.readPlan->u.multi.plans[j]->u.simple.triggerRead.enable;
      reader->isStopNTags = false;
      reader->isStopNTags = reader->readParams.readPlan->u.multi.plans[j]->u.simple.stopOnCount.stopNTriggerStatus;
      reader->numberOfTagsToRead = reader->readParams.readPlan->u.multi.plans[j]->u.simple.stopOnCount.noOfTags;
    }

    switch(op)
    {
    case TMR_SR_OPCODE_READ_TAG_ID_SINGLE :
      {
        ret = TMR_SR_msgSetupReadTagSingle(msg, &i, subProtocol,metadataFlags, filter[j], subTimeout);
        break;
      }
    case TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE:
      {
        //simple read plan uses this funcution, only when tagop is NULL,
        //s0, need to check for simple read plan tagop.
        if (TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)
        {
          //check for the tagop
          if (NULL != reader->readParams.readPlan->u.multi.plans[j]->u.simple.tagop)
          {
            uint32_t readTimeMs = (uint32_t)subTimeout;
            uint8_t lenbyte = 0;
            //add the tagoperation here
            ret = TMR_SR_addTagOp(reader,reader->readParams.readPlan->u.multi.plans[j]->u.simple.tagop,
              reader->readParams.readPlan->u.multi.plans[j], msg, &i, readTimeMs, &lenbyte);
          }
          else
          {
            ret = TMR_SR_msgSetupReadTagMultipleWithMetadata(reader, msg, &i, subTimeout, antennas, metadataFlags ,filter[j], subProtocol, 0);
          }
        }
        else
        {
          if (NULL != reader->readParams.readPlan->u.simple.tagop)
          {
            uint32_t readTimeMs = (uint32_t)subTimeout;
            uint8_t lenbyte = 0;
            //add the tagoperation here
            ret = TMR_SR_addTagOp(reader, reader->readParams.readPlan->u.simple.tagop,
              reader->readParams.readPlan, msg, &i, readTimeMs, &lenbyte);
          }
          else
          {
            ret = TMR_SR_msgSetupReadTagMultipleWithMetadata(reader, msg, &i, subTimeout, antennas, metadataFlags ,filter[j], subProtocol, 0);
          }
        }
        break;
      }
    default :
      {
        return TMR_ERROR_INVALID_OPCODE;
      }
    }

    msg[PLenIdx]= i - PLenIdx - 2; //PLEN
    msg[1]=i - 3;
  }
  return ret;
  }


  TMR_Status TMR_SR_cmdMultipleProtocolSearch(TMR_Reader *reader,TMR_SR_OpCode op,TMR_TagProtocolList *protocols, TMR_TRD_MetadataFlag metadataFlags,TMR_SR_SearchFlag antennas, TMR_TagFilter **filter, uint16_t timeout, uint32_t *tagsFound)
  {
  TMR_Status ret; //create ret
  uint8_t msg[TMR_SR_MAX_PACKET_SIZE]; //create msg
  TMR_SR_SerialReader *sr; //setup serial connection?

  sr = &reader->u.serialReader; //link to reader
   tagsFound = 0; //zero out

  //setup a search
  //need to look up init of protocols, flags, antennas, filter
  ret = TMR_SR_msgSetupMultipleProtocolSearch(reader, msg, op, protocols, metadataFlags, antennas, filter, timeout);
  if (ret != TMR_SUCCESS)
  {
    return ret;
  }

  if (op == TMR_SR_OPCODE_READ_TAG_ID_SINGLE)
  {
    uint8_t opcode;

    sr->opCode = op;
    ret = TMR_SR_sendMessage(reader, msg, &opcode, timeout);
    if (TMR_SUCCESS != ret)
    {
      return ret;
    }
    sr->tagsRemaining = 1;
  }

  if (op == TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE)
  {
    sr->opCode = op;
    if(reader->continuousReading)
    {
      uint8_t opcode;
      ret = TMR_SR_sendMessage(reader, msg, &opcode, timeout);
      if (TMR_SUCCESS != ret)
      {
        return ret;
      }
      sr->tagsRemaining=1;
    }
    else //if reader is not already setup for continious reading, do so?
    {

      ret = TMR_SR_sendTimeout(reader, msg, timeout);
      if (TMR_ERROR_NO_TAGS_FOUND == ret)
      {
        sr->tagsRemaining = *tagsFound = 0;
        return ret;
      }
      else if ((TMR_ERROR_TM_ASSERT_FAILED == ret) ||
        (TMR_ERROR_TIMEOUT == ret))
      {
        return ret;
      }
      else if (TMR_SUCCESS != ret)
      {
        uint16_t remainingTagsCount;
        TMR_Status ret1;

        //Check for the tag count (in case of module error)
        ret1 = TMR_SR_cmdGetTagsRemaining(reader, &remainingTagsCount);
        if (TMR_SUCCESS != ret1)
        {
          return ret1;
        }

         tagsFound = remainingTagsCount;
        sr->tagsRemaining = *tagsFound;
        return ret;
      }

       tagsFound = GETU32AT(msg , 9);
      sr->tagsRemaining = *tagsFound;
    }
  }

  return TMR_SUCCESS;
  }

*/


