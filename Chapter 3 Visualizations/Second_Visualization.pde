import netP5.*;
import oscP5.*;

OscP5 oscP5;
NetAddress address; 
NetAddress chuck;

BufferedReader reader;

PImage img;
float orbitVal;

float x, y, xNext, yNext, xTrunc, xNextTrunc, x2ahead, xFrac, yTrunc, yNextTrunc, y2ahead, xaFrac, yFrac, orbitTrunc, orbitNextTrunc, orbitFrac; //interpolation variables


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

String[] line = new String[1024*1024];
float[] pix = new float[1024*1024];
byte b1; 
byte b2;

void setup() {
  size(1024, 1024);
  // Images must be in the "data" directory to load correctly
  //img = loadImage("terrainTri.jpg");
  //img.loadPixels();
  //byte[] fileInput = loadBytes("sine16b.raw"); //input terrain file
  
  reader = createReader("terrain32.txt");
  
  parse();
  
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
  //image(img, 0, 0);
  for(int x = 0; x < width; x++){
    for(int y = 0; y < height; y++){
      set(x, y, displayTerrain[x][y]);
    }
  }
  
  stroke(255, 0, 0);
  updateOrbit();
  
  //float pixelLocation; //location of our point in the pixel array
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

void updateOrbit(){
  xOffset = map(mouseX, 0, 1024, -1.0, 1.0);
  yOffset = map(mouseY, 0, 1024, -1.0, 1.0);
  
  for(int i = 0; i < orbitRes; i++){
    float theta = (PI*2/orbitRes);
    float angle = theta*i;
    orbitX[i] = (radius*cos(angle)) + xOffset;
    orbitY[i] = (radius*sin(angle)) + yOffset;
    //println("x: ", orbitX[i], "y: ", orbitY[i]);
  }
}

void mousePressed(){
  for(int i = 0; i < orbitRes; i++){
    sendOsc(floatAr[i]);
    println(floatAr[i]);  
  }
}

void mouseWheel(MouseEvent event) {
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

void parse(){
  try {
    for(int i = 0; i < pix.length; i++){
      line[i] = reader.readLine();
      pix[i] = float(line[i]);
      //println(line[i]);
    }
    reader.close();
  } 
  catch (IOException e) {
    e.printStackTrace();
  }

}
