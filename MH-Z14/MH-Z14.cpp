/*
Author: Rik Jansen
Date: 22 January 2017

With this library you can read the CO2 level from a MH-Z14 CO2 sensor via a serial UART connection.
The connection for this is using the SoftwareSerial library. By default the pin for RX is 3, for TX is 4. To use
other pins define CO_2_UART_RX and CO_2_UART_TX before including the library (so above #include "MH-Z14.h")

There are two ways to read from the sensor.
1. By using a callback mechanism
Set a callback using setCallback()
Request a read with readRequest(). 
Make sure the process() is handled in loop()
When the read is done, the callback is called with the values read.

2. By waiting for the read to be ready
In some situations you might want to wait for the read to be ready (e.g. battery powered applications) where you want to return to sleep afterwards.
In that case you can use the readAndWait() function. In worst case situations (when the sensor is not responding) it could take a second before a timeout is 
raised.

Two values are read from the sensor, ie. the CO2 value and the temperature. The temperature is not well documented and I assume that is the internal temperature of the sensor.
The sensor updates its values approximately each 30 seconds, so that would be the minimum read time that should be taken into account in the sketch.
*/

#include <Arduino.h>
#include <MH-Z14.h>
#include <SoftwareSerial.h>
//#define DEBUG

SoftwareSerial CO2Serial(CO_2_UART_RX, CO_2_UART_TX);  // RX, TX
#ifdef DEBUG
  uint8_t lastReadState;
  uint8_t testData[9] = {0xFF, 0x01, 0x03, 0x84, 0x56, 0x00, 0x00, 0x00, 0xA9};
#endif

/*
Set a read request to the sensor. When done, the callback is called with the result
Returns 0 if ready to read, true if sensor not ready yet (already reading, warming up, error, ...)
*/
uint8_t CO2Sensor::readRequest(void)
{
  uint8_t state;
  
  this->_readRequest = true;
  state = this->getReadStatus();
  switch (state)
  {
    case CO2_ST_IDLE:
    case CO2_ST_READY:
      return false; // ready to read
    
    default:
      return true;  // request is set but sensor not ready yet
  }  
}

/*
Reads the value and returns the result. Could take 1 second when no response is received
If this function is called when the sensor is not warmed up yet, it will timeout
In case of an error the return value will be 0
*/
uint16_t CO2Sensor::readAndWait(void)
{
  uint8_t ready = false;
  uint32_t  timer = millis();
  this->resetError(); // reset initial status
  this->readRequest();
  
  while (!ready)
  {
    this->process();
    if (this->getReadStatus() == CO2_ST_READY)
      ready = true;
    if (millis() - timer > 1000)  // 1 sec timeout
      return(0);  // error
  }
  return(getLastReadValue());
}

/*
Sets callback which will be called when reading is ready
Return values in arguments:
CO2Level: well, the CO2 level read (if no error)
temperature: The temperature is not well documented and I assume that is the internal temperature of the sensor.
error: 0 if no error, -1 no answer, -2 check on data failed, -3 not all data received
*/
uint8_t CO2Sensor::setCallback(uint8_t (*fptr)(uint16_t CO2Level, uint8_t temperature, uint8_t error))
{
  callback = fptr;
}

/* 
Returns the current read status (from enum GetCO2States)
*/
uint8_t CO2Sensor::getReadStatus(void)
{
  return (readState);
}

/* 
Initialises library
*/
void CO2Sensor::begin(void)
{
  CO2Serial.begin(9600);
  warmUpTimer = millis();
  this->readState = CO2_ST_INIT;
  this->warmUpNeeded = true;
}

/* 
Resets any error on the sensor and makes the sensor ready to be read
*/
uint8_t CO2Sensor::resetError(void)
{
  this->readState = CO2_ST_INIT;
  this->readErrors = 0;
  return (0);
}

/*
Returns the last value read correctly
*/
uint16_t CO2Sensor::getLastReadValue(void)
{
  return (CO2Level);
}

/* 
Call this function in a loop to take care of the reading process
*/
void CO2Sensor::process(void)
{
  #ifdef DEBUG
    if (lastReadState != readState)
    {
      Serial.print("Last ");
      Serial.print(lastReadState);
      Serial.print(", now ");
      Serial.println(readState);
      lastReadState = readState;
    }
  #endif
  
  switch (readState)
  {
    case CO2_ST_INIT:
     /*  if (this->warmUpNeeded)
        readState = CO2_ST_WARM_UP;
      else */
        readState = CO2_ST_IDLE;
      readErrors = 0;
      for (uint8_t j = 0; j < 32; j++)
        CO2Serial.read();   // empty buffer
      break;
      
    case CO2_ST_WARM_UP:    // sensor warming up
      if (millis() - warmUpTimer > CO2_WARM_UP_TIME || temperature > CO2_WARM_TEMP)
      {
        this->warmUpNeeded = false;
        readState = CO2_ST_IDLE;
      }
      break;
      
    case CO2_ST_IDLE:
      if (_readRequest)
      {
        _readRequest = 0;
        sendReadRequest(1);
        CO2Timer = millis();
        readState = CO2_ST_GET_DATA;
      }
      break;
    
    case CO2_ST_GET_DATA:
      if (CO2Serial.available())
     // if (1) // testing purposes
      {
        this->readError = readData(&CO2Level, &temperature);
        if (!this->readError) // read ok
        {
          readErrors = 0;
          if (temperature > CO2_WARM_TEMP || millis() - warmUpTimer > CO2_WARM_UP_TIME)
            this->warmUpNeeded = false;
          
          if (this->warmUpNeeded)
          {
            this->readError = CO2_ERR_NOT_WARM; 
            readState = CO2_ST_ERROR;
          }  
          else
          {
            if (callback)
              callback(CO2Level, temperature, 0);
            readState = CO2_ST_READY;
          }
        }
        else
          readState = CO2_ST_ERROR;        
      }
      else if (millis() - CO2Timer > CO2_READ_TIMEOUT)  // initial read timeout, no data received
      {
        readState = CO2_ST_ERROR;
        this->readError = CO2_ERR_NO_ANS; 
      }
      break;
    
    case CO2_ST_READY:
      readState = CO2_ST_IDLE;
      break;
      
    case CO2_ST_ERROR:
      if (callback)
        callback(0, 0, this->readError);  // report error
      if (++readErrors > 0) // number of errors before flushing the buffer
      {
        #ifdef DEBUG
          Serial.println("Flushing");
        #endif
        for (uint8_t j = 0; j < 32; j++)
          CO2Serial.read();   // empty buffer        
        readErrors = 0;
      } 
      readState = CO2_ST_IDLE;
      break;
    
    default:
      readState = CO2_ST_ERROR;
      this->readError = CO2_ERR_NOT_WARM; 
      break;
    
  }
}

void CO2Sensor::sendReadRequest(uint8_t sensorNumber)
{
  uint8_t i, cmd[10];
  cmd[0] = CO2_START_CHAR;
  cmd[1] = sensorNumber;
  cmd[2] = CO2_READ_CMD;
  for (i = 3; i < sizeof(cmd); i++)
    cmd[i] = 0;
  cmd[8] = calculateCheckByte(cmd, 8);
  #ifdef DEBUG
    Serial.print("Sending to sensor: ");
  #endif
  for (i = 0; i < 9; i++)
  {
    CO2Serial.write(cmd[i]);
    #ifdef DEBUG    
      Serial.print(cmd[i], HEX);
      Serial.print(" ");
    #endif
  }
  #ifdef DEBUG    
    Serial.println();
  #endif
}

uint8_t CO2Sensor::calculateCheckByte(uint8_t* data, uint8_t length)
{
  uint8_t result = 0, i;
  for (i = 0; i < length; i++, data++)
    result += *data;
  return (~result);
}

int8_t CO2Sensor::readData(uint16_t* result, uint8_t* temp)
{
  uint8_t i, bfr[10];
  unsigned long readTimeOut;
  int res;
  for (i = 0; i < 10; i++)
    bfr[i] = 0;
  
  readTimeOut = millis();
  #ifdef DEBUG
    Serial.print("Reading from sensor: ");
  #endif
  // testData[8] = calculateCheckByte(testData, 8);  // testing purposes 
  for (i = 0; i < 9; i++)
  { 
    if (CO2Serial.available())
      bfr[i] = CO2Serial.read(); 
      
      //bfr[i] = testData[i]; // testing purposes
    #ifdef DEBUG
      Serial.print(bfr[i], HEX);
      Serial.print(" ");
    #endif
    delay(1); // somehow this delay is needed to read the check character correctly
    
    if (millis() - readTimeOut > CO2_READ_TIMEOUT)  // not all characters received
      return CO2_ERR_MISS_DATA; // timeout
  }
  
  if (calculateCheckByte(bfr, 8) != bfr[8])
    return CO2_ERR_CHECK; // check failed

  *result = ((int)bfr[2] << 8) + bfr[3];
  *temp = bfr[4];
  #ifdef DEBUG
    Serial.print(" Result: ");
    Serial.print(*result, DEC);
    Serial.print(" temp: ");
    Serial.print(*temp, DEC);
    Serial.println();
  #endif
 
  return 0;
}

