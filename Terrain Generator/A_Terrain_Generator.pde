int TERRAIN_SIZE = 1024; //Size of terrain

PrintWriter output;

float[] pix; //1D array that the terrain will be condensed into
float[][] terrain = new float[TERRAIN_SIZE][TERRAIN_SIZE]; //original terrain
float pointVal = 0;


void setup(){
  
  size(1024, 1024);
  output = createWriter("TESTINT.txt"); //Creating the file to save to

  pix=new float[width*height]; 
  
  calc(); //Calculate the terrain's values
  
  for(int i=0;i<pix.length;i++)
  {
    pix[i] = terrain[i%width][i/width];                                   //Slot each point in the terrain into 1D array
    int inted = int(map(pix[i], -1.0, 1.0, -32768, 32767));               //Convert the floats to fixed point values
    output.print(inted);                                                  //Write the coordinate to the file
    output.println(",");                                                  //Add a comma and line break
    set(i%width,i/width,color(int(map(inted, -32768, 32767, 0, 255))));   //show on screen, 8bit.
    println(inted);                                                       //Print value to monitor
  }
  output.flush(); // Writes the remaining data to the file
  output.close(); // Finish the file write
}


void draw(){}

//---------------------FUNCTIONS
void calc(){
  for(int x = 0; x < TERRAIN_SIZE; x++){                                       //Nested for loop to write to 2D array point by point
    for(int y = 0; y < TERRAIN_SIZE; y++){
      float ex = mapf(x, 0, 1024, -1.0, 1.0);                                 //Map the x and y values from 0-1024 to -1.0 - 1.0...
      float why = mapf(y, 0, 1024, -1.0, 1.0);                                //...This just makes the maths easier 
      pointVal = sin((x/1024.0)*(2*PI))*sin((y/1024.0)*(2*PI));               //Formula for 2DSINE test terrain
      //pointVal = cos(4*atan(why+1/ex)-12*sin(sqrt(sq(ex)-2*ex+sq(why)+1))); //Formula from Stuart James' 2005 appendix - VERIFIED
      terrain[x][y] = pointVal;                                               //Write the calculated value to its relevant spot in the terrain array
    }
  }
}

//---------------------UTILITIES

//Map function for doubles
double mapdub(double x, double in_min, double in_max, double out_min, double out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Map function for floats
float mapf(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
