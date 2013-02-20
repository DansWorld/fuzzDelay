fuzzDelay README
================

fuzzDelay records mono/stereo audio through a delay line and distortion.

========================================================================

Usage:		$ ./fuzzDelay outfile.wav

To compile:

	1. enter the source directory:
			 $ cd .../fuzzDelay/source
	2. use make:
			 $ make

(NOTE) before compiling, you may want to check
       out the macros at the very top of fuzzDelay.

===================================================

For this small command line app, I wanted to use
the Portaudio API to build a realtime effect processor.
 
Learning the API was easy, its the effect design and DSP I know close to nothing about.

The distortion algorithm came from Phil Burk in Portaudio documentation.
The delay algorithm came from Richard Dobson in the APB. (completely explained to my satisfaction)

=================================================================================

Soon I will add breakpoint functionality for the following parameters:
1. right/left channel delay times -> for variable delay effect (doppler)
2. right/left channel delay feedback
3. left/right PANNING (not implemented yet) -> for stereo panning just the delay?



I left the compiled version of portaudio in a static library
Its tricky to build so I HOPE it works on your machine? I'm not sure how that works...
The version I included was compiled on MAC OSX 10.8.2
=================================================================================

Dan Moore
Berklee College of Music
dhmoore@berklee.edu
========================
