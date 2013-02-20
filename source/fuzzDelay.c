// FUZZ DELAY
// this CommandLineApp will distort and delay your input audio, not only outputting it in realtime,
// but also writing it to soundfile when complete

// current problems: the portsf peak information is weird...
//						I must be retrieving it incorrectly

#include <stdio.h>
#include <stdlib.h>
#include "portsf.h"
#include "portaudio.h"
#include "fxdelay.h"


// EFFECT PARAMETERS	
#define NUM_SECONDS (20)	// after NUM_SECONDS, fuzzDelay will turn off and spit out a recording
#define MAX_DELAY_TIME (3.0)	
#define LEFT_DELAY_TIME (2.0) 	// in seconds
#define RIGHT_DELAY_TIME (0.9)	// these params SCREAM breakpoint files!
#define LEFT_FEEDBACK (0.3)		// try between 0 and 1 for feedback
#define RIGHT_FEEDBACK (0.2)
#define LEFT_DRY_WET (0.5)		// 0.0 for dry signal, 1.0 for wet
#define RIGHT_DRY_WET (0.5)


// Important Audio Engine Params
#define DEFAULT_BUFFER_SIZE (1024)
#define SAMPLE_RATE (44100)
#define NUM_CHANNELS (2)
#define PA_SAMPLE_TYPE paFloat32
#define SAMPLE_SILENCE (0.0f)
typedef float SAMPLE;

typedef struct {
	int frameIndex;
	int maxFrameIndex;
	SAMPLE *recordedSamples;
	FXDELAY *delayLeft;
	FXDELAY *delayRight;
} paTestData;

enum {ARG_PROGNAME, ARG_OUTFILE, ARG_NARGS};




/* The CubicAmplifier function I found in portaudio documentation.
	There are 4 different MACRO functions defined below, each one
	further nested with a distortion algorithm. Try em all!	*/
float CubicAmplifier(float input);
//#define FUZZ(x) CubicAmplifier(CubicAmplifier(CubicAmplifier(CubicAmplifier(x))))
#define FUZZ(x) CubicAmplifier(CubicAmplifier(CubicAmplifier(x)))
//#define FUZZ(x) CubicAmplifier(CubicAmplifier(x))
//#define FUZZ(x) CubicAmplifier(x)



static int audioCallback(const void *inputBuffer, void *outputBuffer,
						unsigned long framesPerBuffer,
						const PaStreamCallbackTimeInfo *timeInfo,
						PaStreamCallbackFlags statusFlags,
						void *userData);
						
int main(int argc, char * argv[])
{
	PaStreamParameters inputParameters, outputParameters;
	PaStream *stream;
	PaError err = paNoError;
	PaDeviceInfo *info = NULL;
	PaHostApiInfo *hostapi = NULL;
	paTestData data;
	int numDevices;
	int id;
	int i;
	int totalFrames;
	int numSamples;
	int numBytes;
	int error = 0;
	SAMPLE max, val;
	double average;
	
	// Delay objects (no modulation?!?!?!?!)
	FXDELAY *delL = NULL, *delR = NULL;
	
	
	// Soundfile vars
	PSF_PROPS outprops;
	psf_format outformat = PSF_FMT_UNKNOWN;
	int ofd = -1;
	PSF_CHPEAK *peaks = NULL;
	double peaktime;
	
	
	if(argc != ARG_NARGS) {
		printf("error: insufficient arguments\n");
		return 1;
	}
	
	delL = new_fxdelay();
	if(!delL) {
		printf("error: unable to create a new delay\n");
		error++; goto exit;
	}
	if(fxdelay_init(delL, SAMPLE_RATE, MAX_DELAY_TIME)) {
		printf("error: unable to init delayline\n");
		error++; goto exit;
	}
	
	if(NUM_CHANNELS == 2) {
		delR = new_fxdelay();
		if(!delR) {
			printf("error: unable to create a new delay line\n");
			error++; goto exit;
		}
		if(fxdelay_init(delR, SAMPLE_RATE, MAX_DELAY_TIME)) {
			printf("error: unable to init delay line\n");
			error++; goto exit;
		}
	}
		
	if(psf_init()) {
		printf("error: unable to init portsf\n");
		return 1;
	}
	outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
	if(outformat == PSF_FMT_UNKNOWN) {
		printf("error: %s has unknown format\n", argv[ARG_OUTFILE]);
		error++; goto exit;
	}
	outprops.format = outformat;
	outprops.chans = NUM_CHANNELS;
	outprops.srate = SAMPLE_RATE;
	outprops.samptype = PSF_SAMP_16;
	outprops.chformat = STDWAVE;
	ofd = psf_sndCreate(argv[ARG_OUTFILE], &outprops, 0, 0, PSF_CREATE_RDWR);
	if(ofd < 0) {
		printf("error: unable to create %s\n", argv[ARG_OUTFILE]);
		error++; goto exit;
	}
	peaks = (PSF_CHPEAK *) malloc(sizeof(PSF_CHPEAK) * NUM_CHANNELS);
	if(!peaks) {
		puts("error: no memory for peaks\n");
		error++; goto exit;
	}
	
	
	data.maxFrameIndex = totalFrames = NUM_SECONDS * SAMPLE_RATE;
	data.frameIndex = 0;
	numSamples = totalFrames * NUM_CHANNELS;
	numBytes = numSamples * sizeof(SAMPLE);
	data.recordedSamples = (SAMPLE *) malloc(numBytes);
	if(data.recordedSamples == NULL) {
		printf("error: could not allocate record array\n");
		error++; goto exit;
	}
	for(i = 0; i < numSamples; i++) data.recordedSamples[i] = 0;
	data.delayLeft = delL;
	data.delayRight = delR;
	
	err = Pa_Initialize();
	if(err != paNoError) {
		printf("error: unable to init potaudio\n");
		error++; goto exit;
	}
	
	printf("INPUT DEVICE:\n");
	numDevices = Pa_GetDeviceCount();
	for(i = 0; i < numDevices; i++) {
		info = (PaDeviceInfo *)Pa_GetDeviceInfo(i);
		hostapi = (PaHostApiInfo *) Pa_GetHostApiInfo(info->hostApi);
		if(info->maxInputChannels > 0) {
			printf("%d: [%s] %s (input)\n", i, hostapi->name, info->name);
		}
	}
	printf("Please type a number and press enter: ");
	scanf("%d", &id);
	if(id < 0 || id > numDevices) {
		printf("error: invalid device number\n");
		error++; goto exit;
	}
	inputParameters.device = id;
	inputParameters.channelCount = NUM_CHANNELS;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	printf("OUTPUT DEVICE:\n");
	for(i = 0; i < numDevices; i++) {
		info = (PaDeviceInfo *) Pa_GetDeviceInfo(i);
		hostapi = (PaHostApiInfo *) Pa_GetHostApiInfo(info->hostApi);
		if(info->maxOutputChannels > 0) {
			printf("%d: [%s] %s (input)\n", i, hostapi->name, info->name);
		}
	}
	printf("Please type a number and press enter: ");
	scanf("%d", &id);
	if(id < 0 || id > numDevices) {
		printf("error: invalid device number\n");
		error++; goto exit;
	}
	outputParameters.device = id;
	outputParameters.channelCount = NUM_CHANNELS;
	outputParameters.sampleFormat = PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream(&stream,
						&inputParameters,
						&outputParameters,
						SAMPLE_RATE,
						DEFAULT_BUFFER_SIZE,
						paClipOff,
						audioCallback,
						&data);
	if(err != paNoError) {
		printf("error: unable to open audio stream\n");
		error++; goto exit;
	}
	
	err = Pa_StartStream(stream);
	if(err != paNoError) {
		printf("error: unable to start audio stream\n");
		error++; goto exit;
	}
	while((err = Pa_IsStreamActive(stream)) == 1) {
		Pa_Sleep(1000);
		printf("index = %d\n", data.frameIndex); fflush(stdout);
	}
	if(err != paNoError) {
		printf("error: unable to maintain audio stream\n");
		error++; goto exit;
	}
	err = Pa_CloseStream(stream);
	if(err != paNoError) {
		printf("error: unable to close the stream\n");
		error++; goto exit;
	}
	
	
	// Write the buffer to soundfile
	if(psf_sndWriteFloatFrames(ofd, data.recordedSamples, totalFrames) != totalFrames) {
		printf("error: unable to write all the float frames to %s\n", argv[ARG_OUTFILE]);
		error++; goto exit;
	}
	
	if(psf_sndReadPeaks(ofd, peaks, NULL) > 0) {
		printf("PEAK information:\n");
		for(i = 0; i < NUM_CHANNELS; i++)
			peaktime = peaks[i].pos / SAMPLE_RATE;
			printf("CH %d:\t%.4f at %.4f secs\n", i+1, peaks[i].val, peaktime);
	}
	
	printf("DONE! goodbye.\n");
exit:
	if(ofd >= 0) {
		if(psf_sndClose(ofd)) {
			printf("errpr: unable to close %s\n", argv[ARG_OUTFILE]);
			error++;
		}
	}	
	if(psf_finish()) {
		printf("error: unable to closeup portsf\n");
		error++;
	}
	if(peaks) free(peaks);
	if(delL) fxdelay_free(delL);
	if(delR) fxdelay_free(delR);
	Pa_Terminate();
	if(data.recordedSamples)
		free(data.recordedSamples);
	return error;
}

static int audioCallback(const void *inputBuffer, void *outputBuffer,
						unsigned long framesPerBuffer,
						const PaStreamCallbackTimeInfo *timeInfo,
						PaStreamCallbackFlags statusFlags,
						void *userData)
{
	paTestData *data = (paTestData *) userData;
	const SAMPLE *in = (const SAMPLE *) inputBuffer;
	SAMPLE outValue;
	SAMPLE *odac = (SAMPLE *) outputBuffer;
	SAMPLE *record = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
	FXDELAY *delayLeft = data->delayLeft;
	FXDELAY *delayRight = data->delayRight;
	float delayOutLeft, delayOutRight, mixOutLeft, mixOutRight;
	long framesToCalc;
	long i;
	int finished;
	unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
	
	(void) timeInfo;
	(void) statusFlags;
	
	if(framesLeft < framesPerBuffer) {
		framesToCalc = framesLeft;
		finished = paComplete;
	} else {
		framesToCalc = framesPerBuffer;
		finished = paContinue;
	}
	
	if(!inputBuffer) {
		for(i = 0; i < framesToCalc; i++) {
			*record++ = SAMPLE_SILENCE;
			*odac++ = SAMPLE_SILENCE;
			if(NUM_CHANNELS == 2) {
				*record++ = SAMPLE_SILENCE;
				*odac++ = SAMPLE_SILENCE;
			}
		}
	} else {
		for(i = 0; i < framesToCalc; i++) {
			if(NUM_CHANNELS == 1) {
				delayOutLeft = fxdelay_tick(delayLeft, LEFT_DELAY_TIME, LEFT_FEEDBACK, FUZZ(*in));
				mixOutLeft = 0.5 * FUZZ(*in++) + 0.5 * delayOutLeft;
				*odac++ = mixOutLeft;
				*record++ = mixOutLeft;
			} else if(NUM_CHANNELS == 2) {
				delayOutLeft = fxdelay_tick(delayLeft, LEFT_DELAY_TIME, LEFT_FEEDBACK, FUZZ(*in));
				mixOutLeft = (1.0 - LEFT_DRY_WET) * FUZZ(*in++) + LEFT_DRY_WET * delayOutLeft;
				delayOutRight = fxdelay_tick(delayRight, RIGHT_DELAY_TIME, RIGHT_FEEDBACK, FUZZ(*in));
				mixOutRight = (1.0 - RIGHT_DRY_WET) * FUZZ(*in++) + RIGHT_DRY_WET * delayOutRight;
				*odac++ = mixOutLeft;
				*odac++ = mixOutRight;
				*record++ = mixOutLeft;
				*record++ = mixOutRight;
			}	
		}
	}
	data->frameIndex += framesToCalc;
	return finished;
}

float CubicAmplifier(float input)
{
	float output, temp;
	if(input < 0.0) {
		temp = input + 1.0f;
		output = (temp * temp * temp) - 1.0f;
	} else {
		temp = input - 1.0f;
		output = (temp * temp * temp) + 1.0f;
	}
	return output;
}
