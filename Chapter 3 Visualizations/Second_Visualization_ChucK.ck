Impulse im => Gain g => dac;
0.5 => g.gain;

// create our OSC receiver
OscRecv recv;
// use port 12001
12004 => recv.port;
// start listening 
recv.listen();

// create an address in the receiver, store in new variable
recv.event( "/Wavetable, f" ) @=> OscEvent oe;

0 => int updateFlag;
44100 => int sampRate;
44100 => float fSampRate;
440.0 => float targetFreq; //TARGET FREQ
1024 => int resolution;
1024.0 => float fRes;
0.0 => float frInc; //frequency increment
0.0 => float playhead; //NOTE: This is a FLOAT! 

//Wavetables
float wt[resolution]; 
float updateTable[resolution];

//Clear memory 
for(0 => int i; i < resolution; i++){
    0 => updateTable[i] => wt[i];
}

//Fill the wavetable with a sinewave intitally
//This was just used for testing
for(0 => int i; i<resolution; i++){
   Math.sin((i/1024.0)*(2*Math.PI)) => wt[i]; //Sine
   //----------------------------------------------------------------------------------------
   //Math.random2(-1, 1) => wt[i];              //Random Pulse
   //----------------------------------------------------------------------------------------
   //(2.0/resolution)*i-1 => wt[i];             //Basic Saw
   //----------------------------------------------------------------------------------------
   //if(i < resolution/2){-1.0 => wt[i];}       //Basic SQR
   //else if( i >= resolution/2){1.0 => wt[i];}
}

spork~ oscListen(); //Spend 5 seconds listening for a wavetable message

5::second => now;

spork~ sequence();  //Fork the sequence to change the target frequency

updateWavetable();  //Move the new wavetable into the wavetable array

prepareForPlay();   //Reset the playhead and determine the how much to increment it by

//Main loop
while(true){
    cookFrequency();    //re-calculate the frequency increment
    processAudioFrame();    //write the relevant sample to the output
    1::samp => now;
}

//------------------------------------------------------------------------------------------------------------------
//Calculate how much to increment playhead by
fun void cookFrequency(){ 
    //Increment = TableLength*DesiredFrequency/SampleRate
    fRes*targetFreq/fSampRate => frInc;    
}

//Reset playhead to start of wavetable
fun void resetPlayhead(){
    0.0 => playhead;
}

//Do both of the previous functions at once
fun void prepareForPlay(){
    resetPlayhead();
    cookFrequency();
}

//Calculate the value of the next sample (including interpolation) and write it to the dac 
fun void processAudioFrame(){
    int truncPlayheadNext;
    float output;
    int truncPlayhead; //variable for truncated playehead 
    float frac; // fraction left over after truncation
    playhead $ int => truncPlayhead;
    playhead - truncPlayhead => frac;
    
    //Setup wraparound for playhead
    if(truncPlayhead+1 > 1023){0 => truncPlayheadNext;}
    else{truncPlayhead+1 => truncPlayheadNext;}
    
    lInt(wt[truncPlayhead], wt[truncPlayheadNext], frac) => output; //linearly interpolate between samples
    
    output => im.next; //write the interpolated value to the dac
    
    //increment and wrap playhead
    playhead + frInc => playhead;
    if(playhead > 1024){playhead-1024 => playhead;}
}

//Linear Interpolation Function
fun float lInt(float val1, float val2, float t){
    float out;
    val1 * (1-t) + val2 * t => out;
    return out;
}

//move the wavetable from the recieving array into the wavetable array
fun void updateWavetable(){
    for(0 => int i; i < resolution; i++){
        updateTable[i] => wt[i];
    }
}

//Listen for incoming wavetable and update the array if one is recieved
fun void oscListen(){
    0 => int iterate; //iterator
    while ( true )
    {
        // wait for event to arrive
        oe => now;
        
        // grab the next message from the queue. 
        while ( oe.nextMsg() != 0 )
        { 
            // getFloat fetches the expected float 
            oe.getFloat() => updateTable[iterate];
            
            if(iterate >= 1023){
                //me.exit();
                //updateWavetable();
            }
            else{iterate++;}
        }
    }
}

//Just a sequence to fork so it plays different pitches
fun void sequence(){
    
    [ 40, 32, 54, 54, 38, 20 ] @=> int sequenceList[];
    
    while(true){
        for(0 => int it; it < 6; it++){
            Std.mtof(sequenceList[it]) => targetFreq;
            300::ms => now;
        }
    }
}
