//NOTE: This was a very brief visualization test that I did at the beginning of the process, while I was still figuring wave terrain out.
//As such, this could definitely be implemented more efficiently and clearly, but it exists as a document the of development process.
//See the second visualization and the final Teensy implementation for better implementations. 

import netP5.*;
import oscP5.*;

OscP5 oscp5; //memory for the OSC sender
NetAddress myRemoteLocation; //the address we're sending to

int cols, rows;
int scl = 8; //20 for triangles
int w = 1024;
int h = 1024;
int waveSizeCounter = 0;
float amplitude = 0.5;

//NOTE: At this point in the project, I was using "orbit" to refer to the trajectory (with reference to Roads). 
//I use "trajectory" in the thesis. They mean the same thing, though. 
float[][] orbArray; // 2D array for holding trajectory

float speed = 0.025;
float saveSpeed = 0;
boolean pause = false;

float movingVal = 0;

float[][] terrain;
float[] waveform;
float[] orbitSine;
int[] fakeVector;
float sineMod[];
PVector lastPoint;

int wavelength = 127/1;
int turnaround = wavelength/2;
int multiplier = 1;
int gridCentre;
int xCentre;
int yCentre;
int xOffset;
int yOffset;
int yModulation = 0;
int iterator = 0;


void setup(){
  size(600, 600, P3D);
  //fullScreen(P3D);
  cols = w/scl;
  rows = h/scl;
  gridCentre = xCentre = yCentre = rows/2;
  xOffset = gridCentre - (turnaround/2);
  yOffset = gridCentre/2;
  terrain = new float [cols][rows];
  waveform = new float[127];
  orbitSine = new float[127];
  sineMod = new float[127];
  fakeVector = new int[2];
  lastPoint = new PVector(0,0,0);
  for(int i = 0; i < 127; i++){
    orbitSine[i] = sin(2*PI*i/wavelength); //Creating a sine wave that will be folded over itself to become the trajectory
    sineMod[i] = sin(2*PI*i/(wavelength)); //Creating another sine wave that will modulate the position of the trajectory
  }
  oscp5 = new OscP5(this, 13002); //connect this instance to this program, listen on 13002 
  myRemoteLocation = new NetAddress("localhost", 13003); //open an address for us to send to
}

void draw(){

  movingVal -= speed;
  
  //Calculate terrain:
  float yoffset = movingVal;
   for(int y = 0; y < rows; y++){
    float xoffset = movingVal; 
    for(int x = 0; x < cols; x++){
      //terrain[x][y] = map(noise(xoffset, yoffset), 0, 1, -100, 100); //uncomment for moving terrain based on noise
      terrain[x][y] = map(sin(2*PI*xoffset/cols)*sin(2*PI*yoffset/rows), -1, 1, -100, 100); //uncomment for terrain based on sine waves that moves according to mouse position
      xoffset += map(mouseX, 0, (width/2), 0.001, 0.99999999); 
     }
     yoffset += map(mouseY, 0, (height/2), 0.001, 0.99999999);
    }
    
  background(0);
  stroke(255, 60);
  //P3D stuff:
  translate(width/2, height/2);
  rotateX(PI/3);
  rotateY(PI/50);
  rotateZ(PI/3);
  translate(-w/0.85, -h/1.008, -w/2);
 
  //Draw terrain:
  for(int y = 0; y < rows-1; y++){
    for(int x = 0; x < cols; x++){
      stroke(255);
      point(x*scl, y*scl, terrain[x][y]);
    }
  }
  
  boolean goingUp = false;
  int x = 0;
  //Draw trajectory:
  for(int i = 0; i < 127; i++){
    //Use the trajectory to grab the relevant terrain values and load them into the the wavetable array (waveform[]).
    yOffset = int(yCentre-(yCentre*amplitude));
    fakeVector[0] = int((((orbitSine[i] + 1.0f) * 0.5 * 126) * amplitude) + int(yOffset));  
    fakeVector[1] = x+xOffset; 
    waveform[i] = terrain[int(wrapAroundY(fakeVector[0]))][int(wrapAroundX(fakeVector[1]))];
    stroke(255,0,0); 
    

    if(i%turnaround == 0){goingUp = !goingUp;}

    //draw trajectory to display:
    point(wrapAroundY(fakeVector[0])*scl, 
          wrapAroundX(fakeVector[1])*scl, 
          terrain[int(wrapAroundY(fakeVector[0]))][int(wrapAroundX(fakeVector[1]))]);
    if((lastPoint.x - wrapAroundY(fakeVector[0])*scl) > 900 || 
       (lastPoint.x - wrapAroundY(fakeVector[0])*scl) < -900 || 
       (lastPoint.y - wrapAroundX(fakeVector[1])*scl) > 900 || 
       (lastPoint.y - wrapAroundX(fakeVector[1])*scl) < -900){/*do nothing*/}
    else{
      line(lastPoint.x, 
           lastPoint.y, 
           lastPoint.z, 
           wrapAroundY(fakeVector[0])*scl, 
           wrapAroundX(fakeVector[1])*scl, 
           terrain[int(wrapAroundY(fakeVector[0]))][int(wrapAroundX(fakeVector[1]))]);
    }
    println((lastPoint.x - wrapAroundY(fakeVector[0])*scl));
    lastPoint.set(wrapAroundY(fakeVector[0])*scl, 
                  wrapAroundX(fakeVector[1])*scl, 
                  terrain[int(wrapAroundY(fakeVector[0]))][int(wrapAroundX(fakeVector[1]))]);
    
    if(goingUp == true){x++;}
    else if(goingUp == false){x--;}
  }
  multiplier = 1;
  waveSizeCounter = 0;
  OscMessage myMessage = new OscMessage("/waveform");
  myMessage.add(waveform);
  oscp5.send(myMessage, myRemoteLocation);
  yModulation = int(map(sineMod[iterator], -1.0, 1.0, -127, 127));
  iterator++;
  if(iterator > 126){iterator = 0;}
  calcYMod();
  calcXMod(int(map(sineMod[(iterator)], -1.0, 1.0, -60, 60)));
}

void calcYMod(){
  yCentre = gridCentre + yModulation;
}

void calcXMod(int centre){
  xCentre = centre;
  xOffset = xCentre - (turnaround/2);
}

//functions for wrapping the trajectory around the terrain at various points
int wrapAroundX(int x_){
  if(x_ > 126){x_ = (x_ - 126);}
  else if (x_ < 0){x_ = (x_ + 126);}
  else{}
  return x_;
}

float wrapAroundY(float y_){
  if(y_ > 126){y_ = (y_ - 126);}
  else if (y_ < 0){y_ = (y_ + 126);}
  else{}
  return y_;
}
//--------------------------------------------------------------------------

void keyPressed(){
  if(key == ENTER){  //when using the noise terrain, pressing ENTER pauses movement...
    pause = !pause;
    if(pause == true){
      saveSpeed = speed;
      speed = 0;
    }
    else if(pause == false){
      speed = saveSpeed;
    }
  }
}
