// From the Audio Programming Book

#include <stdio.h>

/* user struct defining a single delay tap */
typedef struct delaytap {
	double taptime; // seconds
	double amp;		// should not be negative
} DELAYTAP;

// used by FXDELAY to track tap indexes - srate relative
typedef struct _multitap {
	unsigned long index;
	double amp;
} MULTITAP;

// delay line object: supports both vdelay and multi-tap usage
typedef struct fx_delay {
	float *dl_buf;
	unsigned long dl_length;
	unsigned long dl_input_index;
	double dl_srate;
	// for multitap delay
	MULTITAP *taps;
	unsigned long ntaps;
} FXDELAY;

// creatioin function for delay object:
FXDELAY * new_fxdelay(void);


// init funcs: return 0 on success, -1 on error
// for variable delay:
int fxdelay_init(FXDELAY *delay, long srate, double length_secs);
// for tapped delay:
int fxdelay_mtinit(FXDELAY  *delay, long srate, DELAYTAP taps[], int ntaps);

// the respective tick functions, encapsulating feedback
float fxdelay_tick(FXDELAY *delay, double delaytime_secs, double feedback, float input);
float fxdelay_mttick(FXDELAY *delay, double feedback, float input);

// destruction function
void fxdelay_free(FXDELAY *delay);

// read time/amp pairs from text file
int get_taps(FILE *fp, DELAYTAP **ptaps);

// get message relating to error reported by get_taps()
const char * taps_getLastError(void);