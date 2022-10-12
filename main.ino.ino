#include <Audio.h>
#include <SD.h>
//#include "utilities.h"
#include "settings.h"
#include "input.h"
#include "output.h"
#include "tasks.h" //Probably don't need this file

bool STARTUP = true;

uint64_t nextTime = 0;
uint64_t nextTime2 = 0;

float sweep1 = -0.99;
float sweep2 = 0.99;

//PATCH--------------------------------------------------------------------------------
AudioSynthTerrainReader  tr1;
AudioSynthWaveformSine   s1;                //Sine oscillator for testing purposes
AudioFilterBiquad        antiAlias;         //A crude way of bandlimiting the output
AudioOutputI2S           dacs1;             //xy=589,157
//AudioControlSGTL5000     sgtl5000_1;     //include for teensy audio sheild testing
AudioConnection          patchCord1(tr1, 0, antiAlias, 0);
AudioConnection          patchCord2(antiAlias, 0, dacs1, 0);
//-------------------------------------------------------------------------------------

//TERRAINS (!!READ ONLY!!)------------------------------------------------------------- NOTE:: GLOBAL
File myFile;
const int chipSelect = BUILTIN_SDCARD;

EXTMEM int16_t terrain[TERRAIN_SIZE][TERRAIN_SIZE]; //Double array
EXTMEM int16_t intTerrain[TERRAIN_SIZE][TERRAIN_SIZE]; //int array
EXTMEM int16_t *pointerToTerrain[TERRAIN_SIZE]; //"Surrogate" - i.e. a pointer to all the terrain's pointers
EXTMEM byte displayTerrain[DISPLAY_TERRAIN_SIZE][DISPLAY_TERRAIN_SIZE]; //PREVIOUSLY EXTMEM
extern int16_t wavetable[TERRAIN_SIZE];
//-------------------------------------------------------------------------------------

//IO OBJECTS---------------------------------------------------------------------------

WTSInput ins;
WTSOutput outs;

//-------------------------------------------------------------------------------------

void setup() {
  //Allocate memory for the Audio Library
  AudioMemory(100);

  //Setup our USB Serial port for debugging
  Serial.begin(9600);
  Serial.println("Begin Setup");
  
  // AudioMemoryUsageMax(); //Use this function to figure out how much audiomemory you should allocate
                            //Teensy forums reckon try double what your max usage is (https://forum.pjrc.com/threads/39245-AudioMemory()-what-parameter-should-I-pass)
 
  //Input Setup
    ins.beginInput(); 
    initTerrain();    

  //DSP Setup
  antiAlias.setLowpass(0, 15000, 0.707); //Setting the antialiasing filter to 15 kHz, no resonance
  tr1.loadTerrain(pointerToTerrain);     //Linking the pointer to the terrain with the DSP submodule
  tr1.begin(70, 1, 0.0, 0.0, 0.3, 1.0);  //initializing the DSP submodule: (freq, orbit type, orbit x pos, orbit y pos, orbit amplitude, terrain amplitude)
  
  //Output Setup   
  //Set header bytes for transfer 
  outs.setHeaders(69, 55, 42); //(terrain, continuous, discrete)
  Serial.println("Setup Finished");

  //tasksInitial();
  delay(3000);
}

//Main Loop
void loop() {
  uint64_t currentTime = millis();

  //The STARTUP routine just transfers the loaded terrain (shrunk to display size), line-by-line, to the display MCU (a.k.a reciever)
  if(STARTUP == true){
    //if in STARTUP state, run this sequence
    outs.listenForReply(); //listen for reciever to tell whether or not it's ready for a new line of the terrain
    outs.transferDiscreteUpdates(true); //transfer that new line
    
    //Once we're done with the startup transfer routine, this function flips to false, which moves us on to the next sequence.
    STARTUP = outs.getStartupState(); 
    
  }
  else{} //Otherwise, move on

  //Perform tasks at 1ms intervals
  if((currentTime - nextTime2) >= INPUT_SPEED_MS){
    nextTime2 += INPUT_SPEED_MS;
    
    //if we're past the startup routine, run the normal stuff
    if(STARTUP == false){
      ins.readSwitches();     //Read the switches
      ins.readCVInputs();     //Read the CV Inputs
      ins.readPots();         //Read the pots
      ins.updateDSPParams();  //Update the DSP submodule 
      inputOutputLink();      //Make sure the input and output submodules are consistent
    }
    else{}
  }

  //Perform tasks at 17ms intervals
  if((currentTime - nextTime) >= CONTINUOUS_UPDATE_SPEED_MS){
    nextTime += CONTINUOUS_UPDATE_SPEED_MS; 
    //If we're past the startup routine, run the normal stuff
    if(STARTUP == false){
      outs.transferContinuousUpdates(); //Send an update about the trajectory to the display MCU
    }
    else{}
  }
}

//INPUT -> DSP -> OUTPUT LINKING-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//This function transfers any live updating params from the input to the output
void inputOutputLink(){
  float postFilter;

  //Update our DSP and output values to reflect the new input values
  tr1.orbitX(ins.paramBank[0]);                   
  tr1.orbitY(ins.paramBank[1]);                     
  tr1.amplitudeOrbit(ins.paramBank[3]);           
  tr1.amplitudeTerrain(ins.paramBank[4]);         
  tr1.updateFrequency(ins.paramBank[5]);            
  tr1.setTrajectoryState(ins.getInputMode());     

  outs.setTrajScale(ins.paramBank[3]);  
  outs.setTrnScale(ins.paramBank[4]);   
  outs.setZ(ins.paramBank[2]);          
  outs.setJoyX(ins.potsScaled[3]);      
  outs.setJoyY(ins.potsScaled[4]);      
  
  //if the output state doesn't match the input state, set the output state to whatever the input state is and then transfer to screen
  if(ins.getInputMode() != outs.getOutputState()){
    outs.setState(ins.getInputMode()); //NOTE: EVERYTHING ABOVE 0 REPRESENTS A FIXED TRAJECTORY. 0 REPRESENTS FREE TRAJECTORY MODE.
    outs.transferDiscreteUpdates(false); //false is because the terrain will not be changing
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//TERRAIN FUNCTIONS----------------------------------------------------------------------------------------------------------------------------------

//Clear the memory allocated for the terrain
void clearTerrainArray(){
  Serial.println("Clearing Terrain");
  for(int x = 0; x < TERRAIN_SIZE; x++){
    for(int y = 0; y < TERRAIN_SIZE; y++){
      terrain[x][y] = 0;  
    }
  }  
  Serial.println("Terrain Cleared");
}

//Read the terrain from the SD card into memory (i.e. the terrain array)
void readTerrain(){
  //open the file for reading:
  myFile = SD.open("TESTINT.txt", FILE_READ);
  if (myFile) {
    Serial.println("Opening TESTINT.txt:");
    
    // read from the file until there's nothing else in it:
    int i = 0;  //iterator
    int j = 0;  //iterator
    String str = "";    
    while (myFile.available()){
      char kar = myFile.read(); //Read a character from the file

      //If the character is a comma, convert whatever's in the string into an int 
      //and store that in its respective position in the terrain.
      //Then iterate the counter
      if(kar == 44){ //44 is the DEC version of the ASCII comma character
        
        //Serial.println(str);
        
        //NOTE: This must be consistent with the Processing Terrain Generator described in section 3.2.3
        terrain[j%TERRAIN_SIZE][j/TERRAIN_SIZE] = str.toInt(); //Splitting the 1D list into its 2D positions
        
        i = 0; 
        str = "";
        j++;
      }
      //If the character is a line break instead, skip it
      else if(kar == 13 || kar == 10){i++;} //13 & 10 are DEC versions of ASCII line break characters
      //If the character is a number, add it to the string
      else{str += kar; i++;}
    }
    // close the file:
    myFile.close();
    
    Serial.println("SD Terrain File All Read!");
  }
   
  else{
    // if the file didn't open, print an error:
    Serial.println("error opening test");
  }
}

void writeTerrainPointers(){
  for (int i = 0; i < TERRAIN_SIZE; ++i){
        pointerToTerrain[i] = terrain[i];
    }  
}

void terrainToDisplaySize(){
  //loop through the big terrain and condense into 32 * 32 displayTerrain 
  for(int x = 0; x < TERRAIN_SIZE; x++){
    for(int y = 0; y < TERRAIN_SIZE; y++){
      if(x%32 == 0 && y%32 == 0){
        byte value;
        value = (byte) map(terrain[x][y], -32768, 32767, 0, 255);
        outs.setTerrainPoint(x/32, y/32, value);
      } 
      else{//do nothing
      }
    }
    //Serial.println();     
  }
}

//Just a printing function for checking things are loading correctly
void printTable(){
   for(int i = 0; i < 1024; i++){
    int j = terrain[256][i];
//    int x = tr1.orbitXArray[i];
//    int y = tr1.orbitYArray[i];
    Serial.println(j);
//    Serial.print(x);
//    Serial.print(", ");
//    Serial.print(y);
//    Serial.println("...");
   }   
}

//An all-in-one function that calls the other functions in sequence and sets everything up in one line
void initTerrain(){
  //Set up the terrain and get ready to go! 
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  clearTerrainArray();    //Clears memory
  readTerrain();          //Reads terrain into memory
  writeTerrainPointers(); //initialize pointer array
  terrainToDisplaySize(); //Copys terrain into smaller size array to be sent to display MCU
  //printTable();
}
