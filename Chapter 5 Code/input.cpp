
#ifndef input_cpp_
#define input_cpp_

//#include <ADC.h>
//#include <ADC_util.h>
//#include <Bounce.h>
//#include <SD.h>
#include "input.h"
#include "settings.h"
//#include "state.h" //Old file, line here for my memory, please ignore
#include "utilities.h"



//CONSTRUCTOR------------------------------------------------------------------------------------------------------------------------------------
WTSInput::WTSInput(){
  //BOUNCE LIB SETUP FOR SWITCHES
  pinMode(SW_LEFT, INPUT_PULLUP);
  pinMode(SW_RIGHT, INPUT_PULLUP);
  pinMode(JOY_SWITCH, INPUT_PULLUP);
}
//-----------------------------------------------------------------------------------------------------------------------------------------------

//BEGIN------------------------------------------------------------------------------------------------------------------------------------------
void WTSInput::beginInput(){
  
  //Reason for the if statements and pointers explained here: https://arduino.stackexchange.com/questions/21913/instantiating-bounce-library-inside-a-class
   
  if(adc == NULL){ adc = new ADC(); } // adc object;
  if(switchLeft == NULL){switchLeft = new Bounce(SW_LEFT, 10);} //10ms Debounce
  if(switchRight == NULL){switchRight = new Bounce(SW_RIGHT, 10);} //10ms Debounce
  if(joySwitch == NULL){joySwitch = new Bounce(JOY_SWITCH, 10);} //10ms Debounce 

  //Need to specify SwitchState as the type for SW_OFF
  joySwitchEnum = SwitchState::SW_OFF;
  previousJoySwitchEnum = SwitchState::SW_OFF;
  leftSwitchEnum = SwitchState::SW_OFF;
  previousLeftSwitchEnum = SwitchState::SW_OFF;
  rightSwitchEnum = SwitchState::SW_OFF;
  previousRightSwitchEnum = SwitchState::SW_OFF;

  INPUT_MODE = InputMode::FIXED_TRAJ; //Initial mode is fixed trajectory
  
  //ADC0 SETUP:
  adc->adc0->setAveraging(32); // set number of averages
  adc->adc0->setResolution(10); // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);// change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED); // change the sampling speed (COULD USE VERY_HIGH_SPEED IF NEEDED)

  //ADC1 SETUP:
  adc->adc1->setAveraging(32); // set number of averages
  adc->adc1->setResolution(10); // set bits of resolution
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED); // change the sampling speed 

  //Some for loops to address the pots and inputs if need for smoothing purposes
  for(int i = 0; i<5; i++){
    //pots[i] = 0;              //for testing
    //potsPrevious[i] = 0;      //for testing
  }
  for(int i = 0; i<7; i++){
    //CVInputs[i] = 0;          //for testing
    //CVInputsPrevious[i] = 0;  //for testing
    //paramBank[i] = 0;         //for testing
    //paramBankPrevious[i] = 0; //for testing
    if(i != 5){
      CVSmoothing[i].setSmoothing(10); //This is for everthing that's not the 1V/Oct pitch input. Double check that tho!!
      paramBankSmoothing[i].setSmoothing(10); //ORIGINAL VALUE FOR BOTH OF THESE (in this if statement) WAS 25!! I've tried to make it a little more responsive at CV rates by weakening the smoothing. If there's a lot of noise again, just crank these back up to 25
    }
    else if(i == 5){ //This is for the 1V/oct pitch input. Probably don't want as much smoothing there. 
      CVSmoothing[i].setSmoothing(10); //ORIGINAL VALUE IS 10, and I kinda like it that way for now. 
      paramBankSmoothing[i].setSmoothing(10);
    }
  }

  scalePotSmoothing.setSmoothing(40); //smoothing setup
  levelPotSmoothing.setSmoothing(40); //smoothing setup
  //joyYSmoothing.setSmoothing(400);
}


//-----------------------------------------------------------------------------------------------------------------------------------------------

//MODE SELECTOR----------------------------------------------------------------------------------------------------------------------------------

//REFERENCE LIST FOR OUTPUT STATE TRANSFER::
//0 = FREE TRAJECTORY MODE
//1 = FIXED TRAJECTORY - CIRCLE


void WTSInput::changeInputMode(){
  if(INPUT_MODE == InputMode::FIXED_TRAJ){
    INPUT_MODE = InputMode::FREE_TRAJ;
  }
  else if(INPUT_MODE == InputMode::FREE_TRAJ){
    INPUT_MODE = InputMode::FIXED_TRAJ;
   }
}

//Function for transferring the Input Mode to the outside world 
byte WTSInput::getInputMode(){
  byte out = 0;
    
  switch(INPUT_MODE){
    case InputMode::FREE_TRAJ:
      out = 0;
    break;
  
    case InputMode::FIXED_TRAJ:
      out = 1;
    break;
  }

   return out;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------

//SWITCHES---------------------------------------------------------------------------------------------------------------------------------------
void WTSInput::readSwitches(){
  //We have enum objects for each button's state & previous state
  
  static uint64_t sw_time, sw_closed, sw_open;
  //Serial.println(joySwitch->update());
  if(sw_time != millis()){ //update every millisecond - no need to update quicker 
    sw_time = millis();
    if(joySwitch->update()){
      //This is verified for my PCB, but note that you might need to switch rising and falling edges around if you use a different circuit. 
      if(joySwitch->fallingEdge()){joySwitchEnum = SwitchState::SW_ON;}
      else if(joySwitch->risingEdge()){joySwitchEnum = SwitchState::SW_OFF;}- 
    }
        
    //No change deteched so we are either holding the switch or not using it
    if(joySwitchEnum == SwitchState::SW_ON || joySwitchEnum == SwitchState::SW_HELD){
      //Switch is closed, accumulate closed variable 
      //Serial.println("CLOSED++");
      sw_closed++;
      sw_open = 0;
    }
    else if(joySwitchEnum == SwitchState::SW_OFF){
      //Switch is open, accumulate open variable
      //Serial.println("OPEN++");
      sw_open++;
      sw_closed = 0;
    }
    else{Serial.println("Problems with switching logic: joySwitchEnumState is neither on nor held nor off??");}
  
    switch(joySwitchEnum){
      case SwitchState::SW_ON:  
        if(sw_closed > 500){ //Enable Shift Mode after held for 500ms (used to be 1 Sec)
          joySwitchEnum = SwitchState::SW_HELD;
          SHIFT_MODE = true; 
        }
        else{//let pass
          //Serial.println("SW_ON");
        }
      break;
  
      case SwitchState::SW_OFF:
        //if state used to be ON, not HELD, then we've detected a quick press and need to switch the INPUT_MODE
        if(previousJoySwitchEnum == SwitchState::SW_ON){
          //switch input mode
          changeInputMode(); //Swap Between Fixed and Free Trajectories 
        }
        //if it used to be HELD, then we're exiting SHIFT MODE and need to enter POST SHIFT MODE
        else if(previousJoySwitchEnum == SwitchState::SW_HELD){
          POST_SHIFT = true;
        }
        
        SHIFT_MODE = false; //Disable Shift Mode instantly no matter what the last state was
        //Serial.println("SW_OFF");
      break;
  
      case SwitchState::SW_HELD:
        //do nothing?
        SHIFT_MODE = true; //Probably don't need this but it's insurance
        //Serial.println("SW_HELD");
      break;
    
  
          
        }
        previousJoySwitchEnum = joySwitchEnum; //The current state is now the former state
    
  }  
}


//POTS-----------------------------------------------------------------------------------------------------------------------------
void WTSInput::readPots(){
  volatile float f;
  volatile float filt;
  
  //SCALE POT
  pots[0] = adc->adc1->analogRead(SLIDE_1); //  Read the pot and update the value at its position in the array
                                            //  ----------COMMENTED OUT LINES ARE OLD CODE HERE FOR MY MEMORY, PLEASE IGNORE------------------------------------------ 
                                            //  f = expMovingAverage((float) pots[0], 0.01, potsPrevious[0]);  //THIS NEEDS A LINEAR MOVING AVERAGE, NOT EXPONENTIAL!!
                                            //  potsPrevious[0] = f;
                                            //  f = LERPflo((float) pots[0], potsPrevious[0], 0.5);
                                            //  potsPrevious[0] = f;
                                            //  ----------------------------------------------------------------------------------------------------------------------
  if(SHIFT_MODE == true){ //if we're in shift mode, we're trying to alter the base frequency
    baseFreqMod = pots[0]>>2; //reduce the bitdepth to 8bit and update the base frequency modulation param
  }
  else if(SHIFT_MODE == false && POST_SHIFT == false){ //if we're not in shift mode and not in post shift either 
    f = scalePotSmoothing.smooth((float)pots[0]);      //smooth the scale pot value
    potsScaled[0] = mapflo(f, 0.0, 1023.0, -0.7, 0.7); //then map it between -0.7 and 0.7
  }
  else if(SHIFT_MODE == false && POST_SHIFT == true){  //If we're in POST_SHIFT mode ignore pot until its value is the same as our course tune
    f = scalePotSmoothing.smooth((float)pots[0]);
    if(mapflo(f, 0.0, 1023.0, -0.7, 0.7) == potsScaled[0]){
      POST_SHIFT = false;  
    }
  }
  
  //FREQ POT :: MAP CONVERTS THIS TO THE 127 MIDI NOTES
  pots[1] = adc->adc1->analogRead(SLIDE_2); 
  if(SHIFT_MODE == true){  
    fineTune = (byte) map(pots[1], 0, 1023, 0, 255);        //Converting our fine tuning to an 8bit number ranging from 0 - 255
  }
  else if(SHIFT_MODE == false && POST_SHIFT == false){  
    potsScaled[1] = (float) map(pots[1], 0, 1023, 0, 127);  //Mapping our course tuning to MIDI notes
  }
  else if(SHIFT_MODE == false && POST_SHIFT == true){       //If we're in POST_SHIFT mode ignore pot until its value is the same as our course tune
    if((float) map(pots[1], 0, 1023, 0, 127) == potsScaled[1]){
      POST_SHIFT = false;  
    }
  }

  //LEVEL POT
  pots[2] = adc->adc1->analogRead(SLIDE_3);                 //Read pot  
  f = (float) levelPotSmoothing.smooth((float)pots[2]);     //Smooth the value  
  potsScaled[2] = mapflo(f, 0.0, 1024.0, -0.500, 0.500);    //Scale it between -0.5 and 0.5

  //JOYSTICK X 
  pots[3] = adc->adc1->analogRead(JOY_X);                        //The joystick values follow the same format but use a different kind of smoothing
  f = expMovingAverage((float) pots[3], 0.01, potsPrevious[3]);  //ORIGINAL SLOWER VALUE == 0.0005
  potsPrevious[3] = f;
  //f = joyXSmoothing.smooth((float)pots[3]);
  potsScaled[3] = mapflo(f, 0.0, 1024.0, -1.0, 1.0);

  //JOYSTICK Y 
  pots[4] = adc->adc1->analogRead(JOY_Y); 
  f = expMovingAverage((float) pots[4], 0.01, potsPrevious[4]);  //ORIGINAL SLOWER VALUE == 0.0005
  potsPrevious[4] = f;
  //f = joyYSmoothing.smooth((float)pots[4]);
  potsScaled[4] = mapflo(f, 0.0, 1024.0, 1.0, -1.0);

//  ////TESTING STUFF 
//  for(int i = 0; i < 5; i++){
//    Serial.print(potsScaled[i]);
//    Serial.print("      ");  
//  }
//  Serial.print(fineTune);
//  Serial.print("      ");
//  Serial.println(" ");
  smoothingIterator++;  
  if(smoothingIterator > LINEAR_SMOOTHING_READ_AMOUNT){
    smoothingIterator = 0;
  }
}

void WTSInput::readCVInputs(){
  volatile float f;

  //ADC0 FOR FIRST TWO
  //X INPUT
  CVInputs[0] = adc->adc0->analogRead(INPUT_1); 
                                                          //----------COMMENTED OUT LINES ARE OLD CODE HERE FOR MY MEMORY, PLEASE IGNORE------------------------------------------ 
                                                          //f = expMovingAverage((float) CVInputs[0], 0.001, CVInputsPrevious[0]);  //THIS ALSO NEEDS A LINEAR MOVING AVERAGE, NOT EXPONENTIAL
                                                          //CVInputsPrevious[0] = f;
                                                          //f = (float) CVInputs[0];
                                                          //----------------------------------------------------------------------------------------------------------------------
  f = CVSmoothing[0].smooth((float) CVInputs[0]);
  CVInputsScaled[0] = mapflo(f, 0.0, 1023.0, -1.0, 1.0);  //1023.0 for 10bit Res

  //Y INPUT
  CVInputs[1] = adc->adc0->analogRead(INPUT_2); 
                                                          //----------COMMENTED OUT LINES ARE OLD CODE HERE FOR MY MEMORY, PLEASE IGNORE------------------------------------------ 
                                                          //f = expMovingAverage((float) CVInputs[1], 0.001, CVInputsPrevious[1]);  //THIS ALSO NEEDS A LINEAR MOVING AVERAGE, NOT EXPONENTIAL
                                                          //CVInputsPrevious[1] = f;
                                                          //f = (float) CVInputs[1];
                                                          //----------------------------------------------------------------------------------------------------------------------
  f = CVSmoothing[1].smooth((float) CVInputs[1]);
  CVInputsScaled[1] = mapflo(f, 0.0, 1023.0, -1.0, 1.0);  //1023.0 for 10bit Res
  
  //ADC1 FROM HERE ON IN
  //Z INPUT
  CVInputs[2] = adc->adc1->analogRead(INPUT_3);           //READ
  //f = (float) CVInputs[2];
  f = CVSmoothing[2].smooth((float) CVInputs[2]);         //SMOOTH
  CVInputsScaled[2] = mapflo(f, 0.0, 1023.0, -1.0, 1.0);  //SCALE

  //TJ SCALE INPUT
  CVInputs[3] = adc->adc1->analogRead(INPUT_4); 
  f = (float) 1023 - CVInputs[3];
  f = CVSmoothing[3].smooth(f);
  CVInputsScaled[3] = mapflo(f, 0.0, 1023.0, 0.0, 1.5);

  //TN SCALE INPUT
  CVInputs[4] = adc->adc1->analogRead(INPUT_5); 
  f = (float) 1023 - CVInputs[4];
  f = CVSmoothing[4].smooth(f);
  CVInputsScaled[4] = mapflo(f, 0.0, 1023.0, 0.0, 0.999);

  //NOTE:: THESE LAST TWO NEED SPECIAL SCALING AND IT PROBABLY GOES HERE!!!
  //1V/OCT INPUT                                     
  CVInputs[5] = 1023 - adc->adc1->analogRead(INPUT_6); 
  f = (float) CVInputs[5];
  f = CVSmoothing[5].smooth(f);
  CVInputsScaled[5] = mapflo(f, 544.0, 1023.0, 0.0, 1023.0); //544 only until we do the trimpot fix!! 

  //LIN FM INPUT
  CVInputs[6] = 1023 - adc->adc1->analogRead(INPUT_7); 
  f = (float) CVInputs[6];
  f = CVSmoothing[6].smooth(f);
  CVInputsScaled[6] = mapflo(f, 0.0, 1023.0, 20.0, 300.0);
}

void WTSInput::updateDSPParams(){
    float f; 
    //LIST OF TASKS THAT NEED TO HAPPEN HERE:: 

    //NOTE: THIS APPROACH CAN BE CHANGED IF A DIFFERENT UX APPROACH IS WANTED. WORKS FOR ME, BUT TEST IT OUT FIRST AND SEE HOW YOU FEEL.
    
    // TRAJECTORY X
    f = CVInputsScaled[0] + potsScaled[3];  
    f = simpleSatF(f, -1.0, 1.0);
    //paramBank[0] = f;
    paramBank[0] = expMovingAverage( f, 0.01, paramBankPrevious[0]);  //THIS HELPS WITH NOISE BUT DECREASES RANGE OF MOVEMENT!
    paramBankPrevious[0] = paramBank[0];
    
    // TRAJECTORY Y
    f = CVInputsScaled[1] + potsScaled[4];                            //READ
    f = simpleSatF(f, -1.0, 1.0);                                     //CLIP/SATURATE
    //paramBank[1] = f;
    paramBank[1] = expMovingAverage(f, 0.01, paramBankPrevious[1]);   //AVERAGE/SMOOTH 
    paramBankPrevious[1] = paramBank[1];                              //MOVE THE CURRENT VALUE INTO TO THE PREVIOUS VALUE
    
    // TRAJECTORY Z - IGNORE FOR NOW
    paramBank[2] = 0; //UNUSED ATM!
    
    // TRAJECTORY SCALE
    f = CVInputsScaled[3] + potsScaled[0];
    f = simpleSatF(f, 0.0, 1.5);
    f = paramBankSmoothing[3].smooth(f);
    paramBank[3] = f;
    
    // TERRAIN AMPLITUDE
    f = CVInputsScaled[4] + potsScaled[2];
    f = simpleSatF(f, 0.0, 1.0);
    f = paramBankSmoothing[4].smooth(f);
    paramBank[4] = f;
    
    // FREQUENCY | V/Oct
    f = calcFreq(CVInputsScaled[5], potsScaled[1], fineTune, baseFreqMod); //NOTE:: CVInputsScaled[5] should actually be CVInputs[5] - ATM this is a temporary fix until I can get a trimpot to finish the hardware solution (see lab book jan 9th)
    f = simpleSatF(f, 0.0, 15000.0);
    paramBank[5] = f;
    
    //LINEAR FM - IGNORE FOR NOW PLS
    paramBank[6] = 0;

//    //  //TEST
//  for(int i = 0; i < 7; i++){
//    Serial.print(paramBank[i]);
//    Serial.print("      ");  
//  }
////  Serial.print(fineTune);
////  Serial.print("      ");
//    Serial.println(" ");
}

float WTSInput::calcFreq(float cvRead, float potRead, byte _fineTune, byte _baseFreqModulation){
    //THIS ALGORITHM IS BASED ON THIS ARTICLE: https://www.thingsmadesimple.com/2020/12/27/waveform-generator/
    float freq;
    float baseFreq = mapflo(_baseFreqModulation, 0, 255, 0.00001, 8.1758); //8.1758 IS MIDI NOTE 0, in Hz. THIS IS EFFECTIVELY A CONSTANT!!
    float midiNote = potRead; //THIS IS WHAT SLIDING THE POT CHANGES
    float _fineTuneFloat = mapflo(_fineTune, 0, 255, -1.0, 1.0); //NOTES:: a) Reduce output range if stepping | b) I *think* my map function is performing the typecast automatically |
    float stepNumber, cvVoltage, oneOctMIDI;

    //Convert our read CV to its corresponding step on the 10bit ADC
    stepNumber = cvRead * 0.00322265625;   //0.00322265625 is how many volts per ADC step at 3.3V 10 bit Resolution. This will be a little out of tune due to the size of the steps. Up to 12 bit if it really sux.
    //Convert this step to estimate the voltage coming into the module
    cvVoltage = stepNumber * 3.03030303030303030303;  //REMEMBER TO CALIBRATE v/OCT WITH INPUT TRIMPOT
    //Calculate our exact midi note +/- fine tuning;
    midiNote = midiNote + _fineTuneFloat;

    //Testing stuff
    //Serial.print(midiNote); 
    //Serial.print("      "); 
    //Serial.print(cvRead);
    //Serial.print("      ");
    
    //Divide our exact midi note down into a "perOctave" value
    oneOctMIDI = midiNote/12.0; //midiNote IS YOUR BASE MIDI NOTE and oneOctMIDI is that divided by the 12 notes in the octave
    //Convert to exponentioal scaling based off our base freq!
    freq = baseFreq * pow(2, (cvVoltage + oneOctMIDI));
    return freq;
}


#endif
