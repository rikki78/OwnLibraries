#ifndef MonitorController_h
#define MonitorController_h

#define CONTROL_REQ_TIMEOUT 5000
#define CONTROL_POLL_TIME   60000L

enum ControllerStates
{
  CTRL_ST_INIT,
  CTRL_ST_REQ_VAL,
  CTRL_ST_WAIT_ANS,
  CTRL_ST_RESP_REC,
  CTRL_ST_NO_RESP
};

class ControllerMonitor {
public:
  
  uint8_t answer(void);
  void monitor(void);
  uint8_t alive();
  uint8_t set_callback(uint8_t (*fptr)(uint8_t data));
  uint8_t (*callback)(uint8_t);
  
private:
  uint32_t lastRequestMsgTime;
  uint32_t lastControllerMsgTime;
  uint8_t monitorState;
  boolean answerRecv;
  boolean status;
  uint8_t retry;

};
#endif