#ifndef tasks_cpp_
#define tasks_cpp_

#include "tasks.h"

#include <Audio.h>
//#include "input.h"
#include "settings.h"

//Tasks to do at setup
void tasksInitial(){

}

//Tasks to do continuously
void tasksContinuous(){
//  //Input Tasks
//  ins.readSwitches();
//  ins.readCVInputs();
//  ins.updateDSPParams();
//
//  //Transfer New Input Params to Output
//  inputOutputLink();
//  
//  //Output Tasks
//  outs.transferContinuousUpdates();
}

//Tasks to do at a rate specified by UPDATE_SPEED_MS
void tasksPeriodic(){
//  //Input Tasks
//  ins.readPots();
//  
//  //Output Tasks
//  
}



#endif
