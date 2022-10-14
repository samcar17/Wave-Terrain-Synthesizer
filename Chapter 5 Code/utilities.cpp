#ifndef utilities_cpp_
#define utilities_cpp_

//Map function for floats
float mapflo(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//map function for doubles
double mapfd(double x, double in_min, double in_max, double out_min, double out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Linear interpolation for floats
float LERPflo(float a, float b, float frac){
  float out; 
  out = a * (1-frac) + b * frac;
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

//Exponential Moving Average algorithm from here: https://www.norwegiancreations.com/2015/10/tutorial-potentiometers-with-arduino-and-filtering/
float expMovingAverage(float input, float coefficient, float lastOut){
    float out;

    out = coefficient*input+(1-coefficient)*lastOut;
    lastOut = out;
    return out;
}

#endif
