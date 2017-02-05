#ifndef CO2Sensor_h
#define CO2Sensor_h

#include <SoftwareSerial.h>

#define CO2_START_CHAR  0xFF
#define CO2_READ_CMD    0x86
#define CO2_WARM_UP_TIME  3 * 60 * 1000L
#define CO2_READ_TIMEOUT  200
#define CO2_WARM_TEMP     58

#define CO2_ERR_NONE    0 // no error
#define CO2_ERR_NO_ANS 0xFF // no answer
#define CO2_ERR_CHECK  0xFE // check on data failed
#define CO2_ERR_MISS_DATA 0xFD // -3 not all data received
#define CO2_ERR_NOT_WARM  0xFC // -4 sensor not warmed up 
#define CO2_ERR_SW_ERR    0xFB // -5 undefined state

#ifndef CO_2_UART_RX
  #define CO_2_UART_RX 3
  #define CO_2_UART_TX 4
#endif

class CO2Sensor {
public:
  uint8_t readRequest(void);
  void process(void);
  void begin(void);
  uint8_t setCallback(uint8_t (*fptr)(uint16_t CO2Level, uint8_t temperature, uint8_t error));
  uint8_t (*callback)(uint16_t CO2Level, uint8_t temperature, uint8_t error);
  uint8_t getReadStatus(void);
  uint16_t getLastReadValue(void);
  uint16_t readAndWait(void);
  uint8_t resetError(void);
  
private:
  void CO2ReadStates(void);
  void sendReadRequest(uint8_t sensorNumber);
  uint8_t calculateCheckByte(uint8_t* data, uint8_t length);
  int8_t readData(uint16_t* result, uint8_t* temp);
  

  uint8_t   readState;
  uint32_t  CO2Timer;
  uint32_t  warmUpTimer;
  uint16_t  CO2Level;
  uint8_t   temperature;
  uint8_t   readErrors;
  uint8_t   _readRequest;
  uint8_t   warmUpNeeded;
  int8_t    readError;

};

enum GetCO2States
{
  CO2_ST_INIT,
  CO2_ST_WARM_UP,
  CO2_ST_IDLE,
  CO2_ST_GET_DATA,
  CO2_ST_READY,
  CO2_ST_ERROR
};



#endif // CO2Sensor_h