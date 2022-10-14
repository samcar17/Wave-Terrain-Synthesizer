import netP5.*; //oscP5 needs this one in order to work
import oscP5.*;

OscP5 oscp5; //Memory for the OSC sender
float oscInValue;

float[] array;
int arraySize = 127;
int[] lastPoint;
float[] waveformLocal;


void setup(){
  size(1020, 600);
  array = new float [arraySize];
  waveformLocal = new float [arraySize];
  for(int i = 0; i < arraySize; i++){
    array[i] = sin(2*PI*i/arraySize);
  }
  lastPoint = new int [2];
  lastPoint[0] = 0;
  lastPoint[1] = 300;
  
  OscProperties op = new OscProperties();
  op.setListeningPort(13003);
  op.setDatagramSize(9220);
  oscp5 = new OscP5(this, op); //(connect to this program, listen on port 12001
  //myRemoteLocation = new NetAddress("127.0.0.1", port);
}

void draw(){
  background(0);            //refresh background
  line(0, 300, 1020, 300);  //draw centreline
  
  //draw array to screen
  for(int i = 0; i < arraySize; i++){
    int fixedPointConv = int(map(waveformLocal[i], -100, 100, 0, 600));
    
    if(i%1 == 0){
      stroke(255);
      point(map(i, 0, 127, 0, 1020), fixedPointConv);
      if(i <= 126 && i > 0){line(lastPoint[0], lastPoint[1], map(i, 0, 127, 0, 1020), fixedPointConv);}
     
      
      lastPoint[0] = int(map(i, 0, 127, 0, 1020));
      lastPoint[1] = fixedPointConv;
    }
  }
}

When a message is recieved, write the data into the waveformLocal array
void oscEvent(OscMessage waveMsg){
    for(int i = 0; i < arraySize; i++){
      waveformLocal[i] = waveMsg.get(i).floatValue();
    }
    println(waveMsg.addrPattern());
  //}
}
