import netP5.*;
import oscP5.*;

OscP5 oscP5;
NetAddress address; 

float[] wt = new float[1024]; //wavetable array
int j = 0;

void setup(){
size(1024, 500);

  //Listening Address
  oscP5 = new OscP5(this,12005);
  address = new NetAddress("127.0.0.1",12006);
}

void draw(){
  background(0); //Refresh black background
  printToScreen(); //print the current array to screen
}

//Update the wavetable array from the incoming message
void oscEvent(OscMessage theOscMessage) {
  
  println("msg: " , theOscMessage.get(0).floatValue());
  wt[j] = theOscMessage.get(0).floatValue();
  j++;
  if(j > 1023){j = 0;}
  //println(j);  
}

//Print the current array to screen
void printToScreen(){
  int pix;
  int pix2;
  for(int i = 0; i < 1024; i++){
    int i2 = i+1; 
    if(i2 > 1023){i2 = 0;}
    pix = int(map(wt[i], -1, 1, 0, 500));
    pix2 = int(map(wt[i2], -1, 1, 0, 500));
    stroke(255);
    point(i, pix);
    if(i < 1023){line(i, pix, i2, pix2);}
  }
}
