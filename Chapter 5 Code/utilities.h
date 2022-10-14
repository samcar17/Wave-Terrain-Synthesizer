#ifndef utilities_h_
#define utilities_h_

//UTILITIES----------------------------------------------------------------------------------------

#define ONE_POLE(out, in, coefficient)   out += (coefficient) * ((in) - out); //Line from Mutable Instruments stmlib, in dsp.h
                                                                            //Seems like lower values lower the cutoff...

#define EMA(out, lastOut, in, coefficient) out = coefficient*in+(1-coefficient)*lastOut //Exponential Moving Average


#define SCALE8_16(x) ((((x + 1) << 16) >> 8) - 1) //Converters 
#define SCALE10_16(x) ((((x + 1) << 16) >> 10) - 1)
#define SCALE12_16(x) ((((x + 1) << 16) >> 12) - 1)

//map function for floats
float mapflo(float x, float in_min, float in_max, float out_min, float out_max);

//map function for doubles
double mapfd(double x, double in_min, double in_max, double out_min, double out_max);

//Linear interpolation for floats
float LERPflo(float a, float b, float frac);

//simple saturation function using if statements
float simpleSatF(float value, float minf, float maxf);

//exponential moving average function: See utilities.cpp for more info
float expMovingAverage(float input, float coefficient, float lastOut);

#endif
