//THESE ARE CONFIRMED PINS FOR LC!
#define TFT_MISO  12
#define TFT_MOSI  11  
#define TFT_SCK   13  
#define TFT_DC   9 
#define TFT_CS   10  
#define TFT_RST  8

//----------------------------
//HARDWARE DEFINES
#define SCREEN_SIZE 128
#define TRN_SIZE 32
#define TRAJ_SIZE 32
#define X_INPUT 15
#define Y_INPUT 16

//----------------------------
//UART DEFINES
#define HEADER_TERRAIN      69
#define HEADER_NEWLINE      70
#define HEADER_CONTINUOUS   55
#define HEADER_DISCRETE     42

//----------------------------
//ADAFRUIT COLOUR LIST//
// Color definitions
#define BLK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHT    0xFFFF
//-----------------------------


//#include <Adafruit_GFX.h>    // Core graphics library
#include <ST7735_t3.h> // Hardware-specific library
#include <ST7789_t3.h> // Hardware-specific library
#include <SPI.h>
#include <Wire.h>
#include <ADC.h>
#include <ADC_util.h>

ADC *adc = new ADC(); // adc object;
ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_RST); //teensy GFX library object

// For 1.54" TFT with ST7789
//ST7789_t3 tft = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);

float p = 3.1415926;
unsigned long currentMillis, startMillis;

//----------------------------------------------------------------------------------------------------STARTUP STUFF

bool STARTUP = true; //Are we in the STARTUP routine? 
bool readyToReadLine = false; //Is our buffer clear and are we ready to read a new line of the terrain?
bool firstTransferTerrainUpdate = true; //Is this the first transfer of the terrain update?
byte lineBuffer[TRN_SIZE];
byte lineCounter; 

//----------------------------------------------------------------------------------------------------TERRAINS & TRAJECTORIES

byte currentTerrain[TRN_SIZE][TRN_SIZE];  //terrain array

byte currentTrajectoryX[TRAJ_SIZE]; //trajectory x array
byte currentTrajectoryY[TRAJ_SIZE]; //trajectory y array
byte lastTrajectoryX[TRAJ_SIZE];    //arrays for storing the previous trajectory's positions
byte lastTrajectoryY[TRAJ_SIZE];
byte lastTrajectoryValue[TRAJ_SIZE];

float trajectoryScale = 0.1;        
float joyOffsetX = 0;         
float joyOffsetY = 0;
float joystick[2];
float cvIns[2];
//float lfoX = -0.99;   //used for testing
//float lfoY = 0.99;

bool isTrajectoryFixed = true; //false == FREE, true == FIXED

//byte testVariable = 0; //used for testing
//----------------------------------------------------------------------------------------------------


void setup(void) {
  
  Serial.begin(9600);     //Serial for printing out tests
  Serial.print("hello!");

  Serial1.begin(31250);   //Serial for comms between MCUs
  Serial1.clear();
  
  adc->adc0->setAveraging(32); // set number of averages. Used to be 16 - changed to 32 to test and see if it smooths jitter? 
  adc->adc0->setResolution(8); // set bits of resolution. EXPERIMENTAL:: TRYING TO READ JUST 8BITS BECAUSE IT'S ALL WE NEED

  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED);// conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED); // the sampling speed
  
  // Teensy Library: Use this initializer if you're using a 1.44" TFT (128x128)
  tft.initR(INITR_144GREENTAB);

  //add offsets to account for my particular screen 
  tft.setRowColStart(3,4);

  Serial.println("init");

  //uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  //time = millis() - time;

  //Serial.println(time, DEC);
  delay(500);

  tft.setRotation(1); //Accounts for orientation of display

  //calcTerrain();

  // "loading" graphic
  tft.fillScreen(ST7735_BLACK);
  drawText("Loading...", ST7735_WHITE);
  //delay(2000);

  //drawTerrain();
  //calcTrajectory();
  //drawTrajectory();
  delay(4000);  //Wait to make sure DSP MCU is all set up and ready to go
  //delay(10000);
  startMillis = millis(); 
}

void loop() {

  if(STARTUP == true){    //If in STARTUP state
    listenForHeader();    //Listen for comms from the DSP MCU
    updateTerrain();      //Use those comms to update the terrain
  }

  else if(STARTUP == false){                  //If not in STARTUP state...
    //Serial.println("STARTUP false");        //Used for testing
    if (Serial1.available()){receiveEvent();} //...and if there's serial data available, then use it to update the trajectory
  }
  
  currentMillis = millis();
  if(currentMillis - startMillis >= 17){      //17 ms refresh rate for ~60 FPS
    if(STARTUP == false){                     //If we're not in STARTUP state
      //read cv inputs
      cvUpdate();                             
      calcTrajectory();                       //Calculate the trajectory based on those inputs
      drawTrajectory();                       //And then draw it to screen
    }
  }
  //currentMillis = millis();                 //OLD: Used for testing
  //if(currentMillis - startMillis >= 100){
    //Serial.println(trajectoryScale);
  //}
}

//-----------------------------------------------------------------------------------------------------------FUNCTIONS
void drawText(const char *text, uint16_t color){
  tft.setCursor(32, 32);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void cvUpdate(){    //Read CV inputs and update the array that stores their current values                                     
  byte x, y;
  //read
  x = adc->adc0->analogRead(X_INPUT);
  y = adc->adc0->analogRead(Y_INPUT);
  //map to floats
  cvIns[0] = mapflo(x, 0, 255, -0.999, 0.999); //x is [0] 
  cvIns[1] = mapflo(y, 0, 255, -0.999, 0.999); //y is [1]
}

int16_t colourPacker(uint8_t r, uint8_t g, uint8_t b) { //Function to use RGB values with the GFX library's colour system
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
//--------------------------------------------------------------------OLD: ONLY HERE FOR MY MEMORY, PLEASE IGNORE
//void colourTester(){
//  for(int i = 0; i < 255; i++){
//    tft.fillScreen(colourPacker(0,0,0));
//    Serial.println(i);
//    delay(10);  
//  }   
//}

//void calcTerrain(){
//  for(int x = 0; x < TRN_SIZE; x++){
//    for(int y = 0; y < TRN_SIZE; y++){
//      currentTerrain[x][y] = map(sin(2*PI*x/TRN_SIZE)*sin(2*PI*y/TRN_SIZE), -1, 1, 255, 0); 
//      //Serial.println(currentTerrain[x][y]);    
//    }  
//  } 
//}
//----------------------------------------------------------------------------------------------------------------
void drawTerrain(){
  tft.fillScreen(RED); //ST7735_BLACK
  
  byte xCurrent, yCurrent, xNext, yNext; //Variables for biLERP

    for(int x = 0; x < SCREEN_SIZE; x++){
      for(int y = 0; y < SCREEN_SIZE; y++){
          //Identify where we are in the 32 x 32 terrain array (i.e. our closest known)
          xCurrent = x/4;
          yCurrent = y/4;
          //These statements do the wraparounds so we don't have a big gap at the edge of the screen
          if(xCurrent+1 > TRN_SIZE-1){xNext = 0;} else{xNext = (x/4)+1;}
          if(yCurrent+1 > TRN_SIZE-1){yNext = 0;} else{yNext = (y/4)+1;}

          //Set the mu values (i.e. these determine where we are between one known point and another - are we 25% of the way, or 75%?)
          float muX = x%4; //Must be a value between 0 & 1
          muX = muX/4;
          float muY = y%4; //Must be a value between 0 & 1
          muY = muY/4;

          //Linearly interpolate in 2 dimensions and then draw the resulting pixel
          byte pointVal = biLERP(currentTerrain[xCurrent][yCurrent], currentTerrain[xNext][yCurrent], muX, currentTerrain[xCurrent][yNext], currentTerrain[xNext][yNext], muX, muY); 
          tft.drawPixel(x, y, colourPacker(pointVal, pointVal, pointVal));
      }
    }
}

void calcTrajectory(){
  float x, y;
  //Add offset params together & saturate
  joyOffsetX = cvIns[0] + joystick[0];
  joyOffsetX = simpleSatF(joyOffsetX, -1.0, 1.0);

  joyOffsetY = cvIns[1] + joystick[1];
  joyOffsetY = simpleSatF(joyOffsetY, -1.0, 1.0);

  //Set Traj Scale to 0.05 if Trajectory is Free
  if(isTrajectoryFixed == false){
    trajectoryScale = 0.01;
  }
  else{} //nothing
  
  //Calculate Trajectory
  for(int i = 0; i < TRAJ_SIZE; i++){
    float theta = (PI*2/TRAJ_SIZE);
    float angle = theta*i;
    
    //calculate the value of one point
    x = (trajectoryScale*cos(angle)) + joyOffsetX;
    y = (trajectoryScale*sin(angle)) + joyOffsetY;

    //Scale it to screen and save it in the array
    currentTrajectoryX[i] = mapflo(x, -1.0, 1.0, 0, 127);
    currentTrajectoryY[i] = mapflo(y, -1.0, 1.0, 0, 127);
  }
}

void drawTrajectory(){  
    
    clearLastTrajectory();
  
    for(int i = 0; i < TRAJ_SIZE; i++){ 
      //get the value of the current trajectory point
      int x = currentTrajectoryX[i];
      int y = currentTrajectoryY[i];

      //Wrap the orbit around if it's too big
      if(x >= (SCREEN_SIZE-1)){x = x-(SCREEN_SIZE-1);}  
      if(x <= 0.0){x = x+(SCREEN_SIZE-1);}
      if(y >= (SCREEN_SIZE-1)){y = y-(SCREEN_SIZE-1);}
      if(y <= 0.0){y = y+(SCREEN_SIZE-1);}
      
      //save this state so it can be reset at the start of the next draw
      lastTrajectoryX[i] = x;
      lastTrajectoryY[i] = y;
      lastTrajectoryValue[i] = getInterpolatedPoint(x, y);
      
      tft.drawPixel(x, y, BLK); //Line not needed in theory, but after testing, it works better with it. 
      tft.drawPixel(x, y, RED);
    }
}

void clearLastTrajectory(){
 
  for(int i = 0; i < TRAJ_SIZE; i++){
    tft.drawPixel(lastTrajectoryX[i], lastTrajectoryY[i], BLK);
    tft.drawPixel(lastTrajectoryX[i], lastTrajectoryY[i], colourPacker(lastTrajectoryValue[i], lastTrajectoryValue[i], lastTrajectoryValue[i]));
  }
}

byte getInterpolatedPoint(byte x, byte y){
    //This function detrmines the terrain value of a single pixel
    
    //Variables we need
    byte out;
    byte xTrunc, yTrunc, xNext, yNext; 
    float muX, muY;
    byte x1, x2, xa1, xa2; 

    //Set up for bilerp: Define truncated and fractional variables
    xTrunc = x/4;
    yTrunc = y/4;
    if(xTrunc+1 > TRN_SIZE-1){xNext = 0;} else{xNext = xTrunc+1;}
    if(yTrunc+1 > TRN_SIZE-1){yNext = 0;} else{yNext = yTrunc+1;}
    muX = x%4/4.0;
    muY = y%4/4.0;

    //slot the variables from our terrain into smaller, easier variables so we don't get the most confuusing biLERP line in the world
    x1 = currentTerrain[xTrunc][yTrunc];
    x2 = currentTerrain[xNext][yTrunc];
    xa1= currentTerrain[xTrunc][yNext];
    xa2= currentTerrain[xNext][yNext];

    //interpolate and return
    out = biLERP(x1, x2, muX, xa1, xa2, muX, muY);
    return out;
}

//----------------------------------------------------------------------------------------------------------INPUTS
void receiveEvent(){

  //while (Serial.available() > 0) {
    byte header = Serial1.read(); //Read the header value of the transfer
    if(header != HEADER_TERRAIN && header != HEADER_CONTINUOUS && header != HEADER_DISCRETE){Serial1.clear(); return;} //If the header's not one of our specified headers, clear the buffer and exit function
    
    
    byte in = 0;
    
    switch(header){
      //NOTE: This case is for future use, ignore for now --------------------------------------------------
      case HEADER_TERRAIN: //If we have an incoming terrain change, do this

        //Won't be recieving this yet...
  
      break;
      //---------------------------------------------------------------------------------------------------
  
      case HEADER_CONTINUOUS: //If we have an incoming update for the continuous params, do this
        //continuous[1] == trajScale
        if (Serial1.available()){
          in = Serial1.read();
          //Serial.print("SCALE: ");
          //Serial.println(in);
          trajectoryScale = mapflo(in, 0, 255, 0.0, 1.5);
        }
  
        //continuous[2] == joyX
        if (Serial1.available()){
          in = Serial1.read();
          //Serial.print("JOY X: ");
          //Serial.println(in);
          joystick[0] = mapflo(in, 0, 255, -1.0, 1.0);
        }
  
        //continuous[3] == joyY
        if (Serial1.available()){
          in = Serial1.read();
          joystick[1] = mapflo(in, 0, 255, -1.0, 1.0);
        }

        //continuous[4] == trnScale
        //in = Serial1.read();
        //Insert processing here... changing the backlight maybe?
       
        //continuous[5] == Z
        //in = Serial1.read();
        //Insert processing here... Not sure what to do with Z yet...
        
        //Once Message Recieved, Clear Serial Input
        Serial1.clear();
        in = 0;
      break;
  
      case HEADER_DISCRETE: //If we have an incoming update for the discrete params, do this 
  
        //discrete[1] == STATE
        if (Serial1.available()){
          in = Serial1.read();
          if(in == 0){isTrajectoryFixed = false;} //0 == FREE TRAJECTORY, so trajectory is NOT fixed
          else if(in == 1){isTrajectoryFixed = true;} //1 == FIXED TRAJECTORY
          else{}
        }
        
        //discrete[2] == trajectory number. NOTE: PARAM CURRENTLY UNUSED
        if (Serial1.available()){
          in = Serial1.read();
          //Insert processing here... Param unused for now
        }

        //Once Message Recieved, Clear Serial Input
        Serial1.clear();
        in = 0;
      break;
   }
    
    
    //trajectoryScale = mapflo(c, 0, 255, 0.0, 1.5);
    //Serial.println(testVariable);
  //}
  //Once Message Recieved, Clear Serial Input
  //Serial1.clear();
}

//Listen for the terrain header to kick off the STARTUP routine
void listenForHeader(){
  if(firstTransferTerrainUpdate == true){                       //If this is the very first transfer/this routine hasn't completed before
    if(Serial1.available() > 0 && Serial1.available() < 2){     //And if the buffer only has one value in it
      byte value = Serial1.read();
      if(value == HEADER_TERRAIN){                              //And if that one value is the terrain header byte
        tft.fillScreen(GREEN);                                  //Line only used for testing, feel free to delete
        Serial.print("TRN HEADER:   ");
        Serial.println(value);
        readyToReadLine = true;                                 //THEN: Say we're ready for a new line   
        firstTransferTerrainUpdate = false;                     //Lock off the flag
        Serial1.clear();                                        //Clear our input buffer in preparation for the new line
        Serial1.write(HEADER_NEWLINE);                          //Tell our DSP MCU that we're ready for the new line 
        Serial1.flush();                                        //Wait to make sure the header transfers correctly and then move on
      }
      else{Serial1.clear();}                                    //ELSE: if we're getting anything else in our buffer, which we SHOULDN'T BE, clear the buffer out. 
    }  
  }
}

void updateTerrain(){
  if(readyToReadLine == true){                                  //IF: We're ready to recieve a new line
    if(Serial1.available() > TRN_SIZE){                         //And our buffer has over 32 bytes in it (it should have 33 with the comma)
      //Serial1.readBytesUntil(',', lineBuffer, TRN_SIZE+1);    //(This was the original line but this function caused dropouts!!!) 
      for(int i = 0; i < 32; i++){                              //Transfer the first 32 bytes into the lineBuffer array
        lineBuffer[i] = Serial1.read();  
      }
      Serial.print("lineCounter:   "); //testing
      Serial.println(lineCounter); //testing
      Serial.print("LINE:   "); //testing
      for(int y = 0; y < TRN_SIZE; y++){                        //Then write this array to the respective line of the terrain
        currentTerrain[lineCounter][y] = lineBuffer[y]; 
        Serial.print(lineBuffer[y]); //testing
        Serial.print(", "); //testing
      }
      Serial.println(); //testing
      Serial1.clear();                                          //Once that's done, clear the input buffer to free up space for the next line
      lineCounter++;                                            //Iterate the line counter in preparation for the next line
      if(lineCounter < TRN_SIZE){                               //If the line counter is still within the bounds of our terrain...
        readyToReadLine = true;                                 //We're still ready to read lines, so keep this true... 
        Serial1.write(HEADER_NEWLINE);                          //...And let the display MCU know that we're ready for a new line
      }
      else if(lineCounter >= TRN_SIZE){                         //BUT, if our line counter is out of the terrain bounds, we're done reading terrain lines
        readyToReadLine = false;                                //...so set the flag to false.
        drawTerrain();                                          //The terrain should be ready to go now, so draw it! 
        STARTUP = false;                                        //And that brings us to the end of the STARTUP routine so set the flag to false and get on with your life
      }
    }
  }
}


//UTILITIES----------------------------------------------------------------------------------------
float mapflo(float x, float in_min, float in_max, float out_min, float out_max){ 
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//map function for doubles
double mapfd(double x, double in_min, double in_max, double out_min, double out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Linear Interpolation Func
float LERP(float val1, float val2, float t){   
  float out;
  out = val1 * (1-t) + val2 * t;
  return out;
}

//2D Linear Interpolation Func
float biLERP(float x1, float x2, float fracx, float xa1, float xa2, float fracxa, float fracy){
  float out, lerp1, lerp2; 
  lerp1 = LERP(x1, x2, fracx); 
  lerp2 = LERP(xa1, xa2, fracxa);
  out = LERP(lerp1, lerp2, fracy);
  return out;
}

//simple saturation function using if statements
float simpleSatF(float value, float minf, float maxf){
  float out;
  if(value >= maxf){out = maxf;}
  else if(value <= minf){out = minf;}
  else{out = value;}
  return out;
}
