//#include <stdint.h>
#include <Arduino.h>
#include "Dimmer.h"

#define MAX_DIMMERS 2

#define CHILD_ID_DIM_1 4
#define CHILD_ID_DIM_2 5
#define CHILD_ID_DIM_VAL_1 6
#define CHILD_ID_DIM_VAL_2 7

#define FADE_DELAY	10
#define DIMMER_DEFAULT_DIMMING  10
#define INVALID_DIMMER 0


static uint8_t  dimmerCount = 0;

const unsigned char logDimConvert[101] = {
	0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 
	3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 
	8, 8, 9, 10, 10, 11, 12, 13, 14, 15, 
	16, 17, 18, 19, 20, 22, 23, 24, 26, 27, 
	29, 30, 32, 34, 35, 37, 39, 41, 43, 45, 
	47, 49, 51, 54, 56, 58, 61, 64, 66, 69, 
	72, 75, 78, 81, 84, 87, 90, 93, 97, 100, 
	104, 108, 111, 115, 119, 123, 127, 131, 136, 140, 
	145, 149, 154, 159, 163, 168, 173, 179, 184, 189, 
	195, 200, 206, 212, 217, 223, 230, 236, 242, 248, 
	255, 
};


static char lastDimmer = 0;

void DimmerClass::Dimmer(void)
{
  if( dimmerCount < MAX_DIMMERS) 
    dimmerIndex = dimmerCount++;  // assign a dimmer index
  else
    dimmerIndex = INVALID_DIMMER ;  // too many dimmers
 #ifdef DEBUG
   Serial.println("Dimmer construct init");
 #endif
}


uint8_t DimmerClass::begin(uint8_t dimmer, uint8_t output)
{
  if (dimmer >= MAX_DIMMERS)
    return true;
  
  
  ind.output = output;
  pinMode(output, OUTPUT);      // sets the LED PWM pin
  lastDimmer = dimmer > lastDimmer ? dimmer : lastDimmer;
  
  ind.init = true; // init done
  return false;
}

uint8_t DimmerClass::setFade(uint8_t fade)
{
  if (dimmerIndex > MAX_DIMMERS)
    return(true);
  
  setFadeIn(fade);
  setFadeOut(fade);
  return false;

}

uint8_t DimmerClass::setFadeIn(uint8_t fade)
{
  if (dimmerIndex > MAX_DIMMERS)
    return(true);
  
  ind.dimPeriod[0] = fade;
  return false;

}

uint8_t DimmerClass::setFadeOut(uint8_t fade)
{
  if (dimmerIndex > MAX_DIMMERS)
    return(true);
  
  ind.dimPeriod[1] = fade;
  return false;

}
uint8_t DimmerClass::setLevel(uint8_t dimValue)
{
  return setLevel(dimValue, 0);
}


uint8_t DimmerClass::setLevel(uint8_t dimValue, uint8_t fade)
{
  if (dimmerIndex > MAX_DIMMERS)
    return(true);

  ind.requestedLevel = dimValue;
  // Adjust incoming level if this is a V_LIGHT variable update [0 == off, 1 == on]
  
  // Clip incoming level to valid range of 0 to 100
  ind.requestedLevel = ind.requestedLevel > 100 ? 100 : ind.requestedLevel;
  ind.requestedLevel = ind.requestedLevel < 0   ? 0   : ind.requestedLevel;
  
  ind.delta = (ind.requestedLevel - ind.currentLevel ) < 0 ? -1 : 1;
  if (fade)
    ind.period = fade;
  else
  {
    if (ind.delta < 0)
      ind.period = ind.dimPeriod[1];
    else
      ind.period = ind.dimPeriod[0];
  }
  #ifdef DEBUG
    Serial.print( dimmerIndex );
    Serial.print( " Changing level to " );
    Serial.print( ind.requestedLevel );
    Serial.print( ", from " ); 
    Serial.println( ind.currentLevel );
  #endif
  ind.previousMillisDimmer = millis();   
}  



void DimmerClass::process()
{
  if (!ind.init) // if not used skip this one
    return; 
    
  if (ind.requestedLevel != ind.currentLevel)
  {
    if (millis() - ind.previousMillisDimmer >= ind.period)
    {
      ind.currentLevel += ind.delta;
      analogWrite( ind.output, (int)(logDimConvert[ind.currentLevel]) );
      ind.previousMillisDimmer = millis();
    }
    
    //if (currentLevel == requestedLevel)
      //Serial.println("reached requested level");
  }
}

