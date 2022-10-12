int TERRAIN_SIZE = 1024;

PrintWriter output;

float[] pix;
float[][] og2D = new float[TERRAIN_SIZE][TERRAIN_SIZE];
float pointVal = 0;


void setup(){
  
  size(1024, 1024);
  output = createWriter("TESTINT.txt");

  pix=new float[width*height];
  
  calc();
  
  for(int i=0;i<pix.length;i++)
  {
    pix[i]=og2D[i%width][i/width];
    int inted = int(map(pix[i], -1.0, 1.0, -32768, 32767));
    output.print(inted); // Write the coordinate to the file
    output.println(",");
    set(i%width,i/width,color(int(map(inted, -32768, 32767, 0, 255)))); //show on screen, 8bit.
    println(inted);
  }
  output.flush(); // Writes the remaining data to the file
  output.close(); // Finishes the file
}


void draw(){}

//---------------------FUNCS
void calc(){
  for(int x = 0; x < TERRAIN_SIZE; x++){
    for(int y = 0; y < TERRAIN_SIZE; y++){
      float ex = mapf(x, 0, 1024, -1.0, 1.0);
      float why = mapf(y, 0, 1024, -1.0, 1.0);
      pointVal = sin((x/1024.0)*(2*PI))*sin((y/1024.0)*(2*PI)); //2D SINE TEST TERRAIN
      //pointVal = cos(4*atan(why+1/ex)-12*sin(sqrt(sq(ex)-2*ex+sq(why)+1))); //JAMES APPENDIX - VERIFIED
      og2D[x][y] = pointVal;
    }
  }
}

double mapdub(double x, double in_min, double in_max, double out_min, double out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
