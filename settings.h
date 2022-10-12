#ifndef settings_h_
#define settings_h_

//RESOLUTIONS--------------------------------------------------------------------------
#define TERRAIN_SIZE 1024
#define TERRAIN_SIZE_1D TERRAIN_SIZE*TERRAIN_SIZE
#define DISPLAY_TERRAIN_SIZE 32
#define CV_UPDATE_PERIOD 1000 //10ms previously
#define XY_UPDATE_PERIOD 46 //23 for usual
#define CONTINUOUS_UPDATE_SPEED_MS 17        
#define INPUT_SPEED_MS         1
#define LINEAR_SMOOTHING_READ_AMOUNT  10                     
//-------------------------------------------------------------------------------------

//INPUTS-------------------------------------------------------------------------------
#define INPUT_1 24 //ADC0
#define INPUT_2 25 //ADC0
#define INPUT_3 26 //EVERY OTHER ANALOGUE INPUT IS ADC1
#define INPUT_4 27
#define INPUT_5 38
#define INPUT_6 39
#define INPUT_7 40
#define JOY_Y 14
#define JOY_X 15
#define JOY_SWITCH 2 
#define SLIDE_1 16
#define SLIDE_2 17
#define SLIDE_3 41
#define SW_LEFT               3
#define SW_RIGHT              4
#define SW_LEFT_RED_LED       31
#define SW_LEFT_GREEN_LED     32
#define SW_RIGHT_RED_LED      29
#define SW_RIGHT_GREEN_LED    30
//-------------------------------------------------------------------------------------

#endif
