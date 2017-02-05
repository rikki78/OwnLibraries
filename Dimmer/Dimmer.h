#ifndef Dimmer_h
#define Dimmer_h

//#include <MySensors.h>  
//#include <core/MyMessage.h>
/*
#include <stdint.h>
*/


typedef struct 
{
  uint8_t isActive : 1;
  uint8_t dimPeriod[2];
  uint8_t period;
  uint8_t output : 5;
  boolean init;
  
  int8_t  delta : 2;
  int16_t currentLevel;
  int16_t requestedLevel;
  long previousMillisDimmer;
} dimmerType;

class DimmerClass {
public:
  uint8_t dimmerIndex;
  dimmerType ind; // individual dimmer values
  
  void Dimmer(void);
  
  uint8_t begin(uint8_t dimmer, uint8_t output);
  uint8_t setFade(uint8_t fade);
  uint8_t setFadeIn(uint8_t fade);
  uint8_t setFadeOut(uint8_t fade);
  uint8_t setLevel(uint8_t dimValue);
  uint8_t setLevel(uint8_t dimValue, uint8_t fade);
  void process();
  
private:

};
#endif