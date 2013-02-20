fuzzDelay README
================

Dan Moore
Berklee College of Music
dhmoore@berklee.edu
========================

Usage:		$ ./fuzzDelay outfile.wav

To compile:

	1. enter the source directory:
			 $ cd .../fuzzDelay/source
	2. use make:
			 $ make

(NOTE) before compiling, you may want to check
out the macros at the very top of fuzzDelay.
You can mess with different effect parameters there :P
===================================================


For this small command line app, I wanted to use
the Portaudio API to build a realtime effect processor.
 
Learning the API was easy, its the effect design and DSP i know close to nothing about.

The distortion algorithm came from Phil Burk in Portaudio documentation.
The delay algorithm came from Richard Dobson in the APB. (completely explained to my satisfaction)

