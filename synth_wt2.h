//PLS NOTE:: WITH THIS VERSION YOU _MUST_ HAVE A THE VARIABLE int16_t intTerrain[1024][1024] 
//DEFINED IN YOUR ARDUINO SKETCH

#ifndef synth_terrainReader_h_
#define synth_terrainReader_h_

#include "AudioStream.h"
#include "Arduino.h"
#include "arm_math.h"
#include "math.h"
#include "synth_wt2_terrain.h"
//#include "pgmspace.h"

//Resolutions
#define TERRAIN_RESOLUTION 1024
#define TRAJECTORY_RESOLUTION 1024
//#define PI 3.14159265

class AudioSynthTerrainReader : public AudioStream
{
public:
        //Trajectory Input State 
        enum TrajectoryInputState{
            TRAJ_FREE,
            TRAJ_CIRCLE
        };

        AudioSynthTerrainReader() : AudioStream(0, NULL){
            //Default Values
            //trajectoryType(TRAJECTORY_AUDIO); //Telling us which trajectory to choose 
            trajectoryX(0.5);
            trajectoryY(0.5); //Setting trajectory x&y to centre of terrain as default
            amplitudeTrajectory(0.01); //Usually 0.5
            amplitudeTerrain(0); //Setting amplitude of trajectory and terrain to default values
            updateFrequency(440.0); //TARGET FREQUENCY DEFAULT - @432Hz ;) 
        }

        //TODO: Go through all these input functions and add some safety logic to make sure you don't max/min them out!! 

        void loadTerrain(int16_t **terrainInput){
            terrainPointer = terrainInput;
            
            //test loop
            for(int i = 0; i < 1024; i++){
                wavetable[i] = *(*(terrainPointer+256)+i);
            }
        }

        void updateFrequency(float targetFreq){
            frequency = targetFreq;
            updateFreqInc(); //Calculate frequency increment to read wavetable with 
        }


        void setTrajectoryState(byte in){
            switch(in){
                case 0:
                    trajectoryInputState = TrajectoryInputState::TRAJ_FREE;
                break;

                case 1:
                    trajectoryInputState = TrajectoryInputState::TRAJ_CIRCLE;
                break;

            }
        }

        void amplitudeTrajectory(float trajAmp){
            _trajectoryAmplitude = trajAmp;
            trajectoryAmplitude = trajAmp;
        }
        
        void amplitudeTerrain(float terrainAmp){
            _terrainAmplitude = terrainAmp;
        }

        void trajectoryX(float trajX){
            _trajectoryX = trajX;
            Xtraj = trajX;
        }

        void trajectoryY(float trajY){
            _trajectoryY = trajY;
            Ytraj = trajY;
        }

        //One func 2 rule them all
        void begin(float targetFreq, byte trajType, float trajX, float trajY, float trajAmp, float terrainAmp){
            playhead = 0.0; //Default Value for Playhead - NOTE:: THIS IS A FLOAT
            frequency = targetFreq;
            freqInc = TERRAIN_RESOLUTION*targetFreq/AUDIO_SAMPLE_RATE_EXACT;
            setTrajectoryState(trajType);
            _trajectoryAmplitude = trajAmp;
            _terrainAmplitude = terrainAmp;
            _trajectoryX = trajX;
            _trajectoryY = trajY;

            for(int i = 0; i < TRAJECTORY_RESOLUTION; i++){
                wavetable[i] = 0;
                wavetablePrevious[i] = 0;
                wavetableInterpolated[i] = 0;

            }
        }

        virtual void update(void);
        virtual void trajectorySetup(void);

        int16_t wavetable[TERRAIN_RESOLUTION];
        int16_t wavetablePrevious[TERRAIN_RESOLUTION];
        int16_t wavetableInterpolated[TERRAIN_RESOLUTION]; 
        double trajectoryXArray[TERRAIN_RESOLUTION];
        double trajectoryYArray[TERRAIN_RESOLUTION];
        float trajectoryAmplitude;
        float Xtraj;
        float Ytraj;
        int matchedCrossing;
        int16_t lastSample;
        TrajectoryInputState trajectoryInputState { TrajectoryInputState::TRAJ_CIRCLE };

        int32_t closestPosition = 0; //Testing

private:
        virtual void updateTrajectory(void);
        virtual void resetPlayhead(void);
        virtual void updateFreqInc(void);
        virtual void prepareForPlay(void);
        int16_t **terrainPointer; 
        float playhead;
        float frequency;
        float freqInc;
        float _trajectoryAmplitude;
        float _trajectoryX;
        float _trajectoryY;
        float _terrainAmplitude;

};

#endif