#include <Arduino.h>
#include "ControllerMonitor.h"

uint8_t ControllerMonitor::answer()
{
  lastControllerMsgTime = millis();
  answerRecv = true;

}

void ControllerMonitor::monitor(void)
{

 switch (monitorState)
 {
    case CTRL_ST_INIT:
      status = true;
      Serial.println("ControllerMonitor init");
      monitorState = CTRL_ST_REQ_VAL;
      break;
      
    case CTRL_ST_REQ_VAL:
      if (callback)
      {
        callback(true);
       // Serial.println("ControllerMonitor callback called");
      }
      lastRequestMsgTime = millis();
      monitorState = CTRL_ST_WAIT_ANS; // wait for answer
      answerRecv = false;
      Serial.println("ControllerMonitor req val");
      
      break;
   
    case CTRL_ST_WAIT_ANS:  // waiting for answer
      if (answerRecv)
      {
        status = true;
        answerRecv = false;
        lastRequestMsgTime = millis();
        Serial.println("ControllerMonitor recv resp, wait for new");
        monitorState = CTRL_ST_RESP_REC; // wait for next request
      }
      else if (millis() - lastRequestMsgTime > CONTROL_REQ_TIMEOUT)  // time out
      {
        if (retry++ > 3)
        {
          monitorState = CTRL_ST_NO_RESP; // not responding
          Serial.println("ControllerMonitor no resp, stop");
        }
        else
        {
          Serial.println("ControllerMonitor no resp, retry");
          monitorState = CTRL_ST_REQ_VAL; // try again
        }
      }
      break;
      
    case CTRL_ST_RESP_REC: // responded, wait for next request
      if (millis() - lastControllerMsgTime > CONTROL_POLL_TIME)
      {
        Serial.println("ControllerMonitor recv resp, request values again");      
        monitorState = CTRL_ST_REQ_VAL; // request again
      }
      break;
      
    case CTRL_ST_NO_RESP: // no response
      status = false;
      monitorState = CTRL_ST_REQ_VAL; // request again
      Serial.println("ControllerMonitor no response recv, request values again");      
      break;
      
         
  }
  
}

uint8_t ControllerMonitor::set_callback(uint8_t (*fptr)(uint8_t data))
{
  callback = fptr;
}

uint8_t ControllerMonitor::alive()
{
  return (this->status);
}