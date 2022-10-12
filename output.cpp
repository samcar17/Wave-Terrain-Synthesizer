#ifndef output_cpp_
#define output_cpp_

#include "output.h"
#include "Arduino.h"
#include "utilities.h"


//CONSTRUCTOR------------------------------------------------------------------------------------------------------------------------------------
WTSOutput::WTSOutput(){
  Serial1.begin(31250); //BAUD RATE FOR HARDWARE SERIAL
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
//
//METHODS----------------------------------------------------------------------------------------------------------------------------------------

void WTSOutput::setHeaders(byte _terrainHeader, byte _continuousHeader, byte _discreteHeader){
  byteTerrainHeader = _terrainHeader;
  continuousParams[0] = _continuousHeader; //Header byte for continuousParams transfer
  discreteParams[0] = _discreteHeader; //Header byte for discreteParams transfer    
}

//Happens at startup:
void WTSOutput::initTransfer(float _trajScale, float _trnScale, float _z, float _joyX, float _joyY, byte _STATE, byte _inputTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE]){
  setTrajScale(_trajScale);
  setTrnScale(_trnScale);
//  //setZ(); //Unused for now
  setJoyX(_joyX);
  setJoyY(_joyY);
  setState(_STATE);
  setTerrain(_inputTerrain);
  //transferAllToScreen(); 
}
//
//Next two methods tranfer everything we've got to the screen
void WTSOutput::transferAllToScreen(){
  transferContinuousUpdates();
  transferDiscreteUpdates(false);
}      
                                                                        //  ----------COMMENTED OUT LINES ARE OLD CODE HERE FOR MY MEMORY, PLEASE IGNORE------------------------------------------ 
                                                                        //void WTSOutput::transferAllToScreenExplicit(float _trajScale, float _trnScale, float _z, float _joyX, float _joyY, byte _STATE, byte _inputTerrain[][]){
                                                                        //  setTrajScale(_trajScale);
                                                                        //  setTrnScale(_trnScale);
                                                                        //  //setZ(); //Unused for now
                                                                        //  setJoyX(_joyX);
                                                                        //  setJoyY(_joyY);
                                                                        //  setState(_STATE);
                                                                        //  setTerrain(_inputTerrain);
                                                                        //  transferAllToScreen();
                                                                        //}
                                                                        // -----------------------------------------------------------------------------------------------------------------------
//Transfer smaller, quicker params to screen for real-time updates
void WTSOutput::transferContinuousUpdates(){
  //PARAM TRANSFER LIST:: 
  //TJ SCALE
  //TN SCALE
  //Z
  //JOY X
  //JOY Y
  //Serial.println(joyX);
  //Package everything in the array
  continuousParams[1] = trajScale; 
  continuousParams[2] = joyX; 
  continuousParams[3] = joyY;
  //continuousParams[4] = trnScale; 
  //continuousParams[5] = z;

  //Send message if we can send without waiting for old stuff to transmit
  //Otherwise wait til next time the function is run
  if(Serial1.availableForWrite() > 4){
    Serial1.write(continuousParams, 4);
    //Serial.print(continuousParams[0]); Serial.print("   "); Serial.print(continuousParams[1]); Serial.print("   "); Serial.print(continuousParams[2]); Serial.print("   "); Serial.print(continuousParams[3]); Serial.println("   ");
    Serial1.flush();
  }
   //TESTING: Try printing trajectoryScale  
} 
//
//Transfer the big stuff, like a change in terrains or trajectories etc...
void WTSOutput::transferDiscreteUpdates(bool _terrainChange){
  isTerrainChanging = _terrainChange;
  //CHECK IF WE'RE DOING A FULL TERRAIN UPDATE
  if(isTerrainChanging == true){
    //If the terrain is changing, and if this is the first transfer of the update, send the header byte to let the reciever know
    if(firstTransferOfTerrainUpdate == true){
      firstTransferOfTerrainUpdate = false; //lock of the flag
      Serial1.write(byteTerrainHeader); //write the header
      Serial.print("Header Written..."); //testing
      Serial.println(byteTerrainHeader); //testing
    }

    //If the reciever is ready for a new line, write the new line
    if(readyForNewLine == true){
      readyForNewLine = false; //lock off the flag

      //Write the line from the terrain into the line buffer
      for(int y = 0; y < DISPLAY_TERRAIN_SIZE; y++){
        lineBuffer[y] = byteTerrain[lineCounter][y];
        Serial.print(lineBuffer[y]); //testing
        Serial.print(", "); //testing
        Serial1.write(lineBuffer[y]); //testing - trying to write line to LC manually
      }
      Serial.println(); //testing

      //Send the line buffer, followed by a comma to end the line
      //Serial1.write(lineBuffer, DISPLAY_TERRAIN_SIZE);                                                      
      Serial1.write(",");
      Serial1.flush();

      //iterate the lineCounter variable ready for the next line
      lineCounter++;
    }
    else{} //ignore

    //if we've readched the end of the terrain transfer, end the startup routine
    if(lineCounter >= 32){
      isTerrainChanging = false;
      firstTransferOfTerrainUpdate = false;
      readyForNewLine = false;
      STARTUP_local = false;   
    }
  }

  else if(isTerrainChanging == false){
    //Package everything in the array - position [0] is already taken by the header byte!
    discreteParams[1] = STATE;
    discreteParams[2] = trajectoryNumber;
    //Send message 
    Serial1.write(discreteParams, 3);
    Serial1.flush();  
  }

  //PARAM LIST::
  //STATE
  //TRAJECTROY TYPE

  //isTerrainChanging = false;
}  

//If we get a response back from the reciever saying they're ready for a new line, set our flag to say so
void WTSOutput::listenForReply(){
  if (Serial1.available() > 0) {
    if(Serial1.read() == newLineHeader){
      readyForNewLine = true;
      //Serial.println("...reply recieved, start on new lines"); //testing
      Serial1.clear();
    }
    else{} //ignore  
  }
}


//All these SET functions need to scale from the floats/ints that come into them into bytes ready for transfer
void WTSOutput::setTrajScale(float _trajScale){ trajScale = (int) mapflo(_trajScale, 0.0, 1.5, 0, 255); }
void WTSOutput::setTrnScale(float _trnScale){ trnScale = (int) mapflo(_trnScale, 0.0, 0.999, 0, 255); }
void WTSOutput::setZ(float _z){ z = mapflo(_z, -1.0, 1.0, 0, 255); }  //Currently Unused
void WTSOutput::setJoyX(float _joyX){joyX = mapflo(_joyX, -1.0, 1.0, 0, 255); }
void WTSOutput::setJoyY(float _joyY){ joyY = mapflo(_joyY, -1.0, 1.0, 0, 255); }
void WTSOutput::setState(byte _STATE){ STATE = _STATE; } //THIS ONE MIGHT NEED TO DO A LOT MORE DEPENDING ON A BUNCH OF THINGS...
byte WTSOutput::getOutputState(){return STATE;}
bool WTSOutput::getStartupState(){return STARTUP_local;}
//
//
void WTSOutput::setTerrain(byte _inputTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE]){
  Serial.println();
  Serial.println("BYTE TERRAIN");
  for(int x = 0; x < DISPLAY_TERRAIN_SIZE; x++){
    for(int y = 0; y < DISPLAY_TERRAIN_SIZE; y++){
      byteTerrain[x][y] = _inputTerrain[x][y];
      Serial.print(byteTerrain[x][y]); Serial.print(", ");
    } 
    Serial.println(); 
  }
  Serial.println("---------------------------------------------------------------------------"); 
}

void WTSOutput::setTerrainPoint(int _x, int _y, byte _value){
  byteTerrain[_x][_y] = _value; 
  //Serial.print(_value); Serial.print("      ctr:      "); Serial.print(counter); Serial.print("     _x:     "); Serial.print(_x); Serial.print("     _y:     "); Serial.print(_y);
  //Serial.println();
  counter++;
}

//
//
//
////-----------------------------------------------------------------------------------------------------------------------------------------------
//
#endif
