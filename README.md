# Wave-Terrain-Synthesizer
---
**NEWS:** Firmware version 1.1 forthcoming.  
- Greatly improves efficieny, fixes a bug with scale control, and adds trajectory morph control via "Z" CV input. 
- Video demo: https://www.youtube.com/watch?v=_I_Bj-A8xkg  
---

This is a repo for the wave terrain synthesizer module described in my masters thesis. 

Note that the repo is currently set up for easy examination, hence the folder names referencing chapters. Post-examination, this will change to facilitate easier public involvement with the project. 

This entire project is open-source. Consider the software and firmware to be under an MIT license and the hardware under a CC BY-NC-SA 4.0 license. 

For video demos of the project, see the youtube playlist: 

https://youtube.com/playlist?list=PLmXGqDVKvxz0DLh1PFfO5Okias4mdqFCo

**NOTE ON CHAPTER 5 CODE:**

Please note that both the synth_wt2.h and synth_wt2.cpp files need to be in the same folder as the rest of the Teensy Audio Library. This is not the case for the other "Chapter 5 Code" files. Additionally, synth_wt2.h must be included in the audio library's Audio.h file. 

