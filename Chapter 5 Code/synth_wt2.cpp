//PLS NOTE:: WITH THIS VERSION YOU _MUST_ HAVE A THE VARIABLE int16_t intTerrain[1024][1024] 
//DEFINED IN YOUR ARDUINO SKETCH

#include "AudioStream.h"
#include "Arduino.h"
#include "synth_wt2.h"
#include "arm_math.h"
#include "utility/dspinst.h"

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//UTILITIES
//map function for floating points
float mapf(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//map function for doubles
double mapdub(double x, double in_min, double in_max, double out_min, double out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//1D Linear Interpolation Func 
int16_t LERP(int16_t val1, int16_t val2, double t){   
  int16_t out;
  out = val1 * (1-t) + val2 * t;
  return out;
}

//2D Linear Interpolation Func
int16_t biLERP(int16_t x1, int16_t x2, double fracx, int16_t xa1, int16_t xa2, double fracxa, double fracy){
  int16_t out, lerp1, lerp2; 
  lerp1 = LERP(x1, x2, fracx); 
  lerp2 = LERP(xa1, xa2, fracxa);
  out = LERP(lerp1, lerp2, fracy);
  return out;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//TO DO: Pretty sure updateTrajectory and trajectorySetup do EXACTLY THE SAME THING! Lol, pls remove one
void AudioSynthTerrainReader::trajectorySetup(){
 for(int i = 0; i < TRAJECTORY_RESOLUTION; i++){
    double theta = (PI*2/TRAJECTORY_RESOLUTION);
    double angle = theta*i;
    trajectoryXArray[i] = (_trajectoryAmplitude*cos(angle)) + _trajectoryX;
    trajectoryYArray[i] = (_trajectoryAmplitude*sin(angle)) + _trajectoryY;
  }
}

void AudioSynthTerrainReader::updateTrajectory(){
  for(int i = 0; i < TRAJECTORY_RESOLUTION; i++){
    double theta = (PI*2/TRAJECTORY_RESOLUTION);
    double angle = theta*i;
    double x, y;
    x = (_trajectoryAmplitude*cos(angle)) + _trajectoryX;
    y = (_trajectoryAmplitude*sin(angle)) + _trajectoryY;
    trajectoryXArray[i] = x;
    trajectoryYArray[i] = y;
  }
}

void AudioSynthTerrainReader::resetPlayhead(){
  playhead = 0.0;
}

void AudioSynthTerrainReader::updateFreqInc(){
  freqInc = TRAJECTORY_RESOLUTION*frequency/AUDIO_SAMPLE_RATE_EXACT; 
}

void AudioSynthTerrainReader::prepareForPlay(){
  resetPlayhead();
  updateFreqInc();
}


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void AudioSynthTerrainReader::update(void)
{
  audio_block_t *block;
  double x, y, xFrac, yFrac;
  int xTrunc, yTrunc, xNextTrunc, yNextTrunc;
  int16_t trajectorySample;
  int16_t wavetableToBlockBuffer[AUDIO_BLOCK_SAMPLES]; //128 sample array output buffer 
  int truncPlayhead, truncPlayheadNext;
  double playheadFrac;
  //extern int16_t intTerrain[TERRAIN_RESOLUTION][TERRAIN_RESOLUTION]; //BUG ALERT: THIS ISN'T WORKING HOW YOU THOUGHT IT DID!!!!!!!

  //int16_t terrainAudioLib[TERRAIN_RESOLUTION][TERRAIN_RESOLUTION]; //not sure if this is referencing the extern one or not? 

  if (_terrainAmplitude == 0) {
    playhead += freqInc * AUDIO_BLOCK_SAMPLES;
    return;
  }
  block = allocate(); //This gives you a FRESH BLOCK
  if (!block) {
    playhead += freqInc * AUDIO_BLOCK_SAMPLES;
    return;
  } 
  
  //We used to update trajectory inside the following loop. That seemed to be a bit too much? 
  if(trajectoryInputState != TRAJ_FREE){updateTrajectory();} //Update Trajectory

  //THIS LOOP READS THE TERRAIN VIA THE TRAJECTORY
  for(int i = 0; i < TRAJECTORY_RESOLUTION; i++){
    //Iterate wavetable[] write pointer 
    //Find TerrainVal[x][y] based on current radius and centre 
    if(trajectoryInputState != TRAJ_FREE){
      x = mapdub(trajectoryXArray[i], -1.0, 1.0, 0, TERRAIN_RESOLUTION); //Map our X & Y values onto our terrain
      y = mapdub(trajectoryYArray[i], -1.0, 1.0, 0, TERRAIN_RESOLUTION);  
    }                                                               
    else if(trajectoryInputState == TRAJ_FREE){                     
      x = mapdub(_trajectoryX, -1.0, 1.0, 0, TERRAIN_RESOLUTION);
      y = mapdub(_trajectoryY, -1.0, 1.0, 0, TERRAIN_RESOLUTION);
    }

    //Wrap the trajectory around if it's too big
    if(x >= (TERRAIN_RESOLUTION-1)){x = x-(TERRAIN_RESOLUTION-1);}  
    if(x <= 0.0){x = x+(TERRAIN_RESOLUTION-1);}
    if(y >= (TERRAIN_RESOLUTION-1)){y = y-(TERRAIN_RESOLUTION-1);}
    if(y <= 0.0){y = y+(TERRAIN_RESOLUTION-1);}

    //Setting us up for biLERP
    xTrunc = int(x); yTrunc = int(y); //creating our truncated ints
    xFrac = x - xTrunc; yFrac = y - yTrunc; //creating our fractionals
    //creating our lookahead including wraparound just in case
    if(xTrunc+1 > 1023){xNextTrunc = 0;} else{xNextTrunc = xTrunc + 1;} 
    if(yTrunc+1 > 1023){yNextTrunc = 0;} else{yNextTrunc = yTrunc + 1;}

    //biLERP  
    trajectorySample = biLERP(*(*(terrainPointer+xTrunc)+yTrunc), 
                                *(*(terrainPointer+xNextTrunc)+yTrunc), xFrac, 
                                *(*(terrainPointer+xTrunc)+yNextTrunc), 
                                *(*(terrainPointer+xNextTrunc)+yNextTrunc), xFrac, yFrac);
    

    //Copy the resulting sample into the corresponding position in our wavetable array
    wavetable[i] = trajectorySample; 
  }
    
    //Interpolate the wavetable with its previous values
    for(int i = 0; i < TRAJECTORY_RESOLUTION; i++){
      //Write an interpolation of these two points into the interpolated array
      //0.5 gives us exactly halfway between the waveforms
      wavetableInterpolated[i] = LERP(wavetablePrevious[i], wavetable[i], 0.5);
      //Save current wavetable to previous wavetable  
      wavetablePrevious[i] = wavetable[i];
    }

   updateFreqInc();
   

//read from wavetable into block buffer with inc determined by freq 
   for(int i = 0; i < AUDIO_BLOCK_SAMPLES; i++){
    
    if(playhead >= TRAJECTORY_RESOLUTION-1){
        playhead = playhead-TRAJECTORY_RESOLUTION;
    } 

    //Setup variables for the frame
    truncPlayhead = int(playhead);
    playheadFrac = playhead - truncPlayhead;
    
    //Setup lookahead for playhead 
    if(truncPlayhead + 1 > TRAJECTORY_RESOLUTION){
        truncPlayheadNext = (truncPlayhead+1) - TRAJECTORY_RESOLUTION;
    }
    else{truncPlayheadNext = truncPlayhead+1;}

    //interpolate between points in wavetable and write to buffer
    wavetableToBlockBuffer[i] = LERP(wavetableInterpolated[truncPlayhead], 
                                    wavetableInterpolated[truncPlayheadNext],  
                                    playheadFrac);
    playhead = playhead + freqInc;
   }

  //OUTPUT LOOP:: simple for loop, integer index rather than 
  //pointers to 32 bit packed data. 
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      // write buffer into to outgoing block
      //terrain amplitude acts as level control
      block->data[i] = wavetableToBlockBuffer[i] * _terrainAmplitude; 
  }
    
    transmit(block, 0);
    release(block);
}
