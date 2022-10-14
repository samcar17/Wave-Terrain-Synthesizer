import netP5.*;
import oscP5.*;

OscP5 oscP5;
NetAddress address; 
NetAddress chuck;

BufferedReader reader;

PImage img;
float orbitVal;

float x, y, xNext, yNext, xTrunc, xNextTrunc, x2ahead, xFrac, yTrunc, yNextTrunc, y2ahead, xaFrac, yFrac, orbitTrunc, orbitNextTrunc, orbitFrac; //interpolation variables

//NOTE: At this point in the project, I was using the term "orbit" to refer to the trajectory (with reference to Roads). 
int orbitRes = 1024;  
float radius = 0.2;
float xOffset = 0.3;
float yOffset = 0.3;
float[] orbitX = new float[orbitRes];
float[] orbitY = new float[orbitRes];
float[][] terrain = new float[orbitRes][orbitRes];
int[][] displayTerrain = new int[orbitRes][orbitRes];

float[] floatAr = new float[orbitRes];

float[] pixelLocationMem = new float[orbitRes];

String line;
float[] pix = new float[1024*1024];
byte b1; 
byte b2;

void setup() {
  size(1024, 1024); //size of display window
  //NOTE: This version of the code used a floating point terrain,
  //not the fixed point one created by the terrain generator in this repo/
  //The generator in this repo can easily generate floating point terrains, though...
  //...as the equations output floating point values...
  //...so you just need to remove the mapping function on it's output :)
  reader = createReader("terrain32.txt"); //load the file to be read. 
  
  parse();  //read the file
  
  //Orbit Setup
  for(int i = 0; i < orbitRes; i++){
    float theta = (PI*2/orbitRes);
    float angle = theta*i;
    orbitX[i] = (radius*cos(angle)) + xOffset;
    orbitY[i] = (radius*sin(angle)) + yOffset;
    //println("x: ", orbitX[i], "y: ", orbitY[i]);
  }
  
 //Terrain Setup
  for(int i=0;i<pix.length;i++){
    
    displayTerrain[i%width][i/width] = color(int(map(pix[i], -1.0, 1.0, 0, 255))); //Reduce to 8bits into the displayTerrain array for display
    terrain[i%width][i/width] = pix[i]; //Full resolution into floats for the audio terrain
  }
  
  //Listening Address
  oscP5 = new OscP5(this,12000);
  //Sending Address - LocalHost
  address = new NetAddress("127.0.0.1",12005);
  chuck = new NetAddress("127.0.0.1",12004);
}

//DRAW LOOP-------------------------------------------------------------------------------------------------------------

void draw() { 
  //draw terrain in the background
  for(int x = 0; x < width; x++){
    for(int y = 0; y < height; y++){
      set(x, y, displayTerrain[x][y]);  
    }
  }
  
  stroke(255, 0, 0);
  updateOrbit(); //update trajectory location
  
  //
  for(int i = 0; i < orbitRes; i++){
    x = map(orbitX[i], -1, 1, 0, width);
    y = map(orbitY[i], -1, 1, 0, height);
    
    //println(x);
    
   if(x >= 1023.0){x = x-1023.0;}
   if(x <= 0.0){x = x+1023.0;}
   if(y >= 1023.0){y = y-1023.0;}
   if(y <= 0.0){y = y+1023.0;}
   
    
    xTrunc = int(x); yTrunc = int(y);
    xFrac = x - xTrunc; yFrac = y - yTrunc; 
    if(xTrunc+1 > 1023){xNextTrunc = 0;} else{xNextTrunc = xTrunc + 1;}
    if(yTrunc+1 > 1023){yNextTrunc = 0;} else{yNextTrunc = yTrunc + 1;}
    
    
    orbitVal = biLERP(terrain[int(xTrunc)][int(yTrunc)], terrain[int(xNextTrunc)][int(yTrunc)], xFrac, terrain[int(xTrunc)][int(yNextTrunc)], terrain[int(xNextTrunc)][int(yNextTrunc)], xFrac, yFrac);
    
    floatAr[i] = orbitVal;  
    point(xTrunc, yTrunc);

  }
}

//----------------------------------------------------------------------------------------------------------------------------------

//1D Linear Interpolation Func (Used in the 2D func)
float LERP(float val1, float val2, float t){  //weighted averaging of multiple points either side of val. Rolling mean-style. 
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

void sendOsc(float input){
  OscMessage myMessage = new OscMessage("/Wavetable");
  
  myMessage.add(input);

  oscP5.send(myMessage, address); 
  oscP5.send(myMessage, chuck); 
}

void updateOrbit(){                           //calculate trajectory
  xOffset = map(mouseX, 0, 1024, -1.0, 1.0);  //add offsets from mouse's location
  yOffset = map(mouseY, 0, 1024, -1.0, 1.0);
  
  for(int i = 0; i < orbitRes; i++){          //polar to cartesian conversion 
    float theta = (PI*2/orbitRes);
    float angle = theta*i;
    orbitX[i] = (radius*cos(angle)) + xOffset;
    orbitY[i] = (radius*sin(angle)) + yOffset;
    //println("x: ", orbitX[i], "y: ", orbitY[i]);
  }
}

void mousePressed(){                  //if the mouse is pressed, send an OSC message with the trajectory's values
  for(int i = 0; i < orbitRes; i++){
    sendOsc(floatAr[i]);
    println(floatAr[i]);  
  }
}

void mouseWheel(MouseEvent event) {   //if the mousewheel is scrolled make the radius bigger or smaller
  float e = event.getCount();
  if(e == 1.0 && radius < 1.99){
    radius = radius + 0.01;
  }
  else if(e == 1.0 && radius >= 1.99){
    radius = 1.99;
  }
  else if(e == -1.0 && radius > 0.01){
    radius = radius - 0.01;
  }
  else if(e == -1.0 && radius <= 0.01){
    radius = 0.01;
  }
}

void parse(){                             //read terrain into memory from file
  try {
    for(int i = 0; i < pix.length; i++){ 
      line = reader.readLine();           //read a line into memory. 
      pix[i] = float(line);               //convert it to a float 
      //println(line[i]);
    }
    reader.close();
  } 
  catch (IOException e) {
    e.printStackTrace();
  }

}
