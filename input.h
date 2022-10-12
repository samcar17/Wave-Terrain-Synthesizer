
#ifndef input_h_
#define input_h_

//#include "Arduino.h"
#include <ADC.h>
//#include <ADC_util.h>
#include <Bounce.h>
//#include <SD.h>
#include "settings.h"

enum class SwitchState{
  SW_ON,
  SW_OFF,  
  SW_HELD
};


enum class InputMode{
  FIXED_TRAJ,  
  FREE_TRAJ
};

//ADC Smoothing Algorithm From the o_C code 
//Could potentially be useful to convert this to a template in future
struct o_C_smoothing{
  float out, smoothing, memory;

  void setSmoothing(float input){
      smoothing = input;
  }

  float smooth(float in){
    out = (memory * (smoothing - 1) + in) / smoothing;
    memory = out;
    return out;
  }
};

//INPUT CLASS
class WTSInput{
  public: 

  WTSInput(); //Constructor

  void beginInput();  //Setup function

  //State
  void changeInputMode();
  byte getInputMode();
  
  
  //METHODS:--------------------------------
  //ADC Input:
  void readSwitches();
  void readPots();           
  void readCVInputs();      
  void updateDSPParams();
  float calcFreq(float cvRead, float potRead, byte _fineTune, byte _baseFreqModulation);

  //Arrays:
  volatile int16_t pots[5];
  volatile float potsPrevious[5];
  volatile int   smoothingIterator = 0;
  o_C_smoothing  scalePotSmoothing, levelPotSmoothing;
  volatile float potsScaled[5];
  volatile int16_t CVInputs[7];
  volatile float CVInputsPrevious[7];
  volatile float CVInputsScaled[7];
  o_C_smoothing CVSmoothing[7];
  volatile float paramBank[7];
  volatile float paramBankPrevious[7];
  o_C_smoothing paramBankSmoothing[7];

  //For 1V/Oct calculation:
  volatile byte fineTune;
  volatile byte baseFreqMod;

  //Switch Input:
  bool joySwitchHeld = false;

  
  private:
  //reason for pointers explained here: https://arduino.stackexchange.com/questions/21913/instantiating-bounce-library-inside-a-class
  ADC *adc; // adc object;
  Bounce *switchLeft; //10ms Debounce
  Bounce *switchRight; //10ms Debounce
  Bounce *joySwitch; //10ms Debounce


  bool SHIFT_MODE = false;  //IMPORTANT: WHEN ADDING SCALE POT SHIFT MODE, WE NEED INDEPENDANT POST_SHIFT BOOLS FOR THE STATE OF EVERY POT USED IN SHIFT MODE
  bool POST_SHIFT = false;  //A mode to deal with sharing pot reads. After shift mode is exited it won't start assigning the pot reads to the non-shift param until the old value is matched.
                            //Inspired by the way Ableton works with MIDI

  //Enums:
  SwitchState joySwitchEnum;
  SwitchState previousJoySwitchEnum;
  SwitchState leftSwitchEnum;
  SwitchState previousLeftSwitchEnum;
  SwitchState rightSwitchEnum;
  SwitchState previousRightSwitchEnum;

  InputMode   INPUT_MODE;
};


#endif
