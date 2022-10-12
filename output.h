#ifndef output_h_
#define output_h_

#include "Arduino.h"
#include "settings.h"

class WTSOutput{
  public: 

  WTSOutput(); //Constructor

  //METHODS:--------------------------------
  //UART Output
  void setHeaders(byte _terrainHeader, byte _continuousHeader, byte _discreteHeader);
  void initTransfer(float _trajScale, float _trnScale, float _z, float _joyX, float _joyY, byte _STATE, byte _inputTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE]); //Happens at startup
  void transferAllToScreen();       //These two tranfer everything we've got to the screen
  void transferAllToScreenExplicit(float _trajScale, float _trnScale, float _z, float _joyX, float _joyY, byte _STATE, byte _inputTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE]);

  void transferContinuousUpdates(); //Transfer smaller, quicker params to screen for real-time updates
  void transferDiscreteUpdates(bool _terrainChange);  //Transfer the big stuff, like a change in terrains or trajectories etc...
  void listenForReply(); //listen for a reply from the display MCU to say that it's buffer is clear and ready for another line of the terrain
  
  //All these SET functions need to scale from the floats/ints that come into them into bytes ready for transfer
  void setTrajScale(float _trajScale);
  void setTrnScale(float _trnScale);
  void setZ(float _z);
  void setJoyX(float _joyX);
  void setJoyY(float _joyY);
  void setState(byte _STATE);
  void setTerrain(byte _inputTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE]);
  void setTerrainPoint(int _x, int _y, byte _value);

  //All the GET functions needed to communicate with the rest of the program 
  byte getOutputState();
  bool getStartupState();

  //States
  enum INPUT_MODE_OUTPUT_CLASS {
    FIXED_TRAJ,  
    FREE_TRAJ  
  };
  
  private:

  byte trajScale;
  byte trnScale;
  byte z = 0; //Might not use this atm
  byte joyX;
  byte joyY;
  byte STATE;
  byte trajectoryNumber;
  byte continuousParams[4]; //Number of params plus a header byte to tell us which set of params are changing! 
  byte discreteParams[3]; //Number of params plus a header byte to tell us which set of params are changing!
  byte byteTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE];
  byte lineBuffer[DISPLAY_TERRAIN_SIZE];
  
  bool isTerrainChanging = false; //false means NO! NO CHANGE OF TERRAIN! True means that YES! TERRAIN IS CHANGING! LOOK BUSY!   
  bool STARTUP_local = true; //this represents the question of "Are we in the startup sequence?", which we are as soon as the device turns on, until we're not
  bool readyForNewLine = false; //is the reciever ready for a new line of the terrain? 
  bool firstTransferOfTerrainUpdate = true; //A variable to represent the first transfer of the terrain sequence
  byte lineCounter = 0;
  byte newLineHeader = 70;

  byte byteTerrainHeader; //Header byte for terrain transfer. Using a sperate variable for it to try and avoid making the terrain any bigger than it needs to be
  //continuousParams[0] = 55; //Header byte for continuousParams transfer
  //discreteParams[0] = 42; //Header byte for discreteParams transfer
  int counter = 0;
};

#endif
