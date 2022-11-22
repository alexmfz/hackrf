/**
 * @file hackrf_sweep.c
 * @author Alejandro
 * @brief 
 * @version 2.0
 * @date 2022-11-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "/home/manolo/hackrf/host/libhackrf/src/hackrf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <fftw3.h>
#include <inttypes.h>

#if defined(__GNUC__)
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#endif

#include <signal.h>
#include <math.h>
#include "fitsio.h"
#include "FitsFolder/generatationFits.h"
#include "Functions/functions.h"
#include "Timer/timer.h"

/**
 * CONSTANTES
 */
#define _FILE_OFFSET_BITS 64

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#define FD_BUFFER_SIZE (8*1024)
#define FREQ_ONE_MHZ (1000000ull)
#define FREQ_MIN_MHZ (0)    /*    0 MHz */
#define FREQ_MAX_MHZ (7250) /* 7250 MHz */

#define FFTMAX 	(8180)
#define FFTSIZE_STD	(14400)
#define CUSTOM_SAMPLE_RATE_HZ (20000000)
#define TRIGGERING_TIMES (3600)
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (15000000) /* 15MHz default */

#define TUNE_STEP (CUSTOM_SAMPLE_RATE_HZ / FREQ_ONE_MHZ)
#define OFFSET 7500000

#define BLOCKS_PER_TRANSFER 16
#define THROWAWAY_BLOCKS 2

#if defined _WIN32
	#define m_sleep(a) Sleep( (a) )
#else
	#define m_sleep(a) usleep((a*1000))
#endif

#define ITERATIONS  10
#define INTERVAL 25e4 //us => 25e4 us = 250 ms
#define MICRO   1e6

/********************/

int result = 0;
float totalDuration = 0; //total duration (should be aroung 15 minutes)
bool success = false;
bool executionControl = true;
int counterSucess = 0;
uint32_t num_sweeps = 0;
int num_ranges = 0;
uint16_t frequencies[MAX_SWEEP_RANGES*2];
int step_count;
unsigned int lna_gain=16, vga_gain=20;
uint32_t freq_min = 0;
uint32_t freq_max = 6000;
float step_value = 0; //(freqmax-freqmin)/nChannels (MHz)

FILE* outfile = NULL;
char* path = NULL;
char pathFits[] = "TFM.fits";
float* samples;
int id_sample = 0;
int flag_initialFreqCaught = 0;

uint32_t byte_count = 0;
volatile uint64_t sweep_count = 0;

struct timeval time_start;
struct timeval t_start;

/*Variables for sweepDuration*/
struct timeval time_now;
struct timeval time_prev;

struct timeval time_preConfig;
struct timeval time_postConfig;

int exit_code = EXIT_SUCCESS;
float sweep_rate = 0;
/*****/

bool amp = false;
uint32_t amp_enable;

bool antenna = false;
uint32_t antenna_enable;

bool binary_output = false;
bool ifft_output = false;
bool one_shot = false;
bool finite_mode = false;
bool sweep_started = false;

int fftSize = 20;
double fft_bin_width;
fftwf_complex *fftwIn = NULL;
fftwf_complex *fftwOut = NULL;
fftwf_plan fftwPlan = NULL;
fftwf_complex *ifftwIn = NULL;
fftwf_complex *ifftwOut = NULL;
fftwf_plan ifftwPlan = NULL;
uint32_t ifft_idx = 0;
float* pwr;
float* window;
hackrf_device* device = NULL;
const char*serial_number = NULL;

int numberOfSteps = 0; //(FMax-FMin)/Width
uint32_t requested_fft_bin_width = 0;
int sampleRate = 0;
	

extern long naxes[2];
static float TimevalDiff(const struct timeval *a, const struct timeval *b) {
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

 bool do_exit = false;

int timerFlag = 0;
extern struct itimerval timer;
extern struct timeval preTriggering;
extern struct timeval postTriggering;
float durationIteration = 0;
float durationSweeps = 0;

#ifdef _MSC_VER
BOOL WINAPI
sighandler(int signum) {
	if (CTRL_C_EVENT == signum) {
		fprintf(stderr, "Caught signal %d\n", signum);
		do_exit = true;
		return TRUE;
	}
	return FALSE;
}
#else
void sigint_callback_handler(int signum)  {
	fprintf(stderr, "hackrf_sweep | sigint_callback_handler |Caught signal %d\n", signum);
	do_exit = true;
}
#endif

/**Timer part**/

/**
 * timer handler
 */
void timerHandler(int sig)
{
    timerFlag = 1;
} 

/**
 * timer trigger
 */

float hackRFTrigger()
{ 
	printf("hackrf_sweep | hackRFTrigger()\n");
	gettimeofday(&preTriggering,NULL);

    if(signal(SIGALRM, timerHandler) == SIG_ERR)
    {
        fprintf(stderr, "timer | hackRFTrigger | Unable to catch alarm signal");
        return 0;
    }
   
    if(setitimer(ITIMER_REAL, &timer, 0) == -1)
    {
        fprintf(stderr, "timer | hackRFTrigger | Error calling timer");
        return 0;
    }
    
    
    gettimeofday(&postTriggering,NULL);    
    durationIteration = 0;
    
	return durationIteration;
}

float logPower(fftwf_complex in, float scale)
{
	float re = in[0] * scale;
	float im = in[1] * scale;
	float magsq = re * re + im * im;
	return (float) (log2(magsq) * 10.0f / log2(10.0f));
}

int rx_callback(hackrf_transfer* transfer) {
	int8_t* buf;
	uint8_t* ubuf;
	uint64_t frequency; /* in Hz */
	int i, j;
	struct tm *fft_time;
	char time_str[50];
	struct timeval usb_transfer_time;
	//int nElements = naxes[0]*naxes[1];

	if(NULL == outfile){// || strstr(pathFits,"fits")==NULL) {
		return -1;
	}

	if(do_exit) {
		return 0;
	}

	gettimeofday(&usb_transfer_time, NULL);
	byte_count += transfer->valid_length;
	buf = (int8_t*) transfer->buffer;
	for(j=0; j<BLOCKS_PER_TRANSFER; j++) 
	{
		ubuf = (uint8_t*) buf;
		if(ubuf[0] == 0x7F && ubuf[1] == 0x7F) 
		{
			frequency = ((uint64_t)(ubuf[9]) << 56) |
			 			((uint64_t)(ubuf[8]) << 48) |
						((uint64_t)(ubuf[7]) << 40) |
						((uint64_t)(ubuf[6]) << 32) | 
						((uint64_t)(ubuf[5]) << 24) |
						((uint64_t)(ubuf[4]) << 16) |
						((uint64_t)(ubuf[3]) << 8) |
						ubuf[2];
		} 
		
		else 
		{
			buf += BYTES_PER_BLOCK;
			continue;
		}
		
		/*
	
		*/

		if (frequency == (uint64_t)(FREQ_ONE_MHZ*frequencies[0])) 
		{	
			success = true;

			if(sweep_started) 
			{
				sweep_count++;
				if(one_shot) {	do_exit = true;	}

				//else if(finite_mode && sweep_count == num_sweeps) {	do_exit = true;	}

				else if(counterSucess == TRIGGERING_TIMES) { do_exit = true; }

			}

			sweep_started = true;
		}

		if(do_exit) {
			return 0;
		}

		if(!sweep_started) {
			buf += BYTES_PER_BLOCK;
			continue;
		}

		if((FREQ_MAX_MHZ * FREQ_ONE_MHZ) < frequency) {
			buf += BYTES_PER_BLOCK;
			continue;
		}
		/* copy to fftwIn as floats */
		buf += BYTES_PER_BLOCK - (fftSize * 2);

		for(i=0; i < fftSize; i++) {
			fftwIn[i][0] = buf[i*2] * window[i] * 1.0f / 128.0f;
			fftwIn[i][1] = buf[i*2+1] * window[i] * 1.0f / 128.0f;
		}
		buf += fftSize * 2;
		fftwf_execute(fftwPlan);

		for (i=0; i < fftSize; i++) {
			pwr[i] = logPower(fftwOut[i], 1.0f / fftSize);
		}

		/*
		* To catch data is needed:
			- A path with extension .fits +
			- timerFlag set to 1 which means that 0.25s passed +
			- frequency should be first frequency or this frecuency was caught
		*/
		if(strstr(pathFits, "fits")!= NULL && timerFlag == 1 && ( frequency == (uint64_t)(FREQ_ONE_MHZ*frequencies[0]) || flag_initialFreqCaught == 1)) 
		{			
			if ( frequency == (uint64_t)(FREQ_ONE_MHZ*frequencies[0])) 
			{ 
				flag_initialFreqCaught = 1; //First time will enter
				fprintf(stderr, "First Frecuency caught. Setting flag to 1\n"); 			
			}
				
			time_t time_stamp_seconds = usb_transfer_time.tv_sec;
			fft_time = localtime(&time_stamp_seconds);
			strftime(time_str, 50, "%Y-%m-%d, %H:%M:%S", fft_time);
			printf("%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
				time_str,
				(long int)usb_transfer_time.tv_usec,
				(uint64_t)(frequency), //First time: 45MhZ
				(uint64_t)(frequency+sampleRate/4), //45MHz + 5MHz
				fft_bin_width,
				fftSize);

			for(i = 0; (fftSize / 4) > i; i++) 
			{
				printf(", %.2f", pwr[i + 1 + (fftSize*5)/8]);
			}

			printf("\n");
			printf("%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
					time_str,
					(long int)usb_transfer_time.tv_usec,
					(uint64_t)(frequency+(sampleRate/2)),
					(uint64_t)(frequency+((sampleRate*3)/4)),
					fft_bin_width,	//	--> 49504,95			
					fftSize);
					
			for(i = 0; (fftSize / 4) > i; i++) 
			{
				printf(", %.2f", pwr[i + 1 + (fftSize/8)]);
			}
			
			printf("\n");
			if ((uint64_t)(frequency+((sampleRate*3)/4)) == freq_max*FREQ_ONE_MHZ) //Where the sweep finished
			{
				counterSucess++;
				printf("hackrf_sweep | rx_callback() | Data Caught. Iteration %d finished\n", counterSucess);			
				fprintf(stderr, "hackrf_sweep | rx_callback() | Data Caught. Iteration %d finished\n", counterSucess);			
				success = false;
				timerFlag = 0; // flag down which means that data was caught
				flag_initialFreqCaught = 0; //Set variable to finish iteration
			}
			/*ese
			{
				time_t time_stamp_seconds = usb_transfer_time.tv_sec;
				fft_time = localtime(&time_stamp_seconds);
				strftime(time_str, 50, "%Y-%m-%d, %H:%M:%S", fft_time);
				printf("%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
					time_str,
					(long int)usb_transfer_time.tv_usec,
					(uint64_t)(frequency), //First time: 45MhZ
					(uint64_t)(frequency+sampleRate/4), //45MHz + 5MHz
					fft_bin_width,
					fftSize);
				for(i = 0; (fftSize / 4) > i; i++) {
					printf(", %.2f", pwr[i + 1 + (fftSize*5)/8]);
				}
				printf("\n");
				printf("%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
						time_str,
						(long int)usb_transfer_time.tv_usec,
						(uint64_t)(frequency+(sampleRate/2)),
						(uint64_t)(frequency+((sampleRate*3)/4)),
						fft_bin_width,	//	--> 49504,95			
						fftSize);
				for(i = 0; (fftSize / 4) > i; i++) {
					printf(", %.2f", pwr[i + 1 + (fftSize/8)]);
				}
				
				printf("\n");
				if ((uint64_t)(frequency+((sampleRate*3)/4)) == freq_max*FREQ_ONE_MHZ) //Where the sweep finished
				{
					printf("hackrf_sweep | rx_callback() | No Capturing data for iteration %d\n", sweep_count);			
					fprintf(stderr, "hackrf_sweep | rx_callback() | No Capturing data for iteration %d\n", sweep_count);			
					success = false;
				}
			}*/

		}
		
	}
			
	return 0;
}

/**
 * Checks the paramsf not possible values to throw an error
 */
static int checkParams(){
	if (lna_gain % 8)
	{
		fprintf(stderr, "hackrf_sweep | checkParams | warning: lna_gain (-l) must be a multiple of 8\n");
	}

	if (vga_gain % 2)
	{
		fprintf(stderr, "hackrf_sweep | checkParams | warning: vga_gain (-g) must be a multiple of 2\n");
	}
	
	if( amp ) 
	{
		if( amp_enable > 1 ) 
		{
			fprintf(stderr, "hackrf_sweep | checkParams | argument error: amp_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) 
	{
		if (antenna_enable > 1) 
		{
			fprintf(stderr, "hackrf_sweep | checkParams | argument error: antenna_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (num_ranges == 0) 
	{
		frequencies[0] = (uint16_t)freq_min;
		frequencies[1] = (uint16_t)freq_max;
		num_ranges++;
	}

	if(binary_output && ifft_output) 
	{
		fprintf(stderr, "hackrf_sweep | checkParams | argument error: binary output (-B) and IFFT output (-I) are mutually exclusive.\n");
		return EXIT_FAILURE;
	}

	if(ifft_output && (1 < num_ranges)) 
	{
		fprintf(stderr, "hackrf_sweep | checkParams | argument error: only one frequency range is supported in IFFT output (-I) mode.\n");
		return EXIT_FAILURE;
	}

	/*
	 * The FFT bin width must be no more than a quarter of the sample rate
	 * for interleaved mode. With our fixed sample rate of 20 Msps, that
	 * results in a maximum bin width of 5000000 Hz.
	 */
	if(fftSize < 4) 
	{
		fprintf(stderr, "checkParams | argument error: FFT bin width (-w) must be no more than 5000000\n");
		return EXIT_FAILURE;
	}

	if(fftSize > 8180) 
	{
		fprintf(stderr, "checkParams | argument error: FFT bin width (-w) must be no less than 2445\n");
		return EXIT_FAILURE;
	}

	printf("hackrf_sweep | checkParams | Success\n");
	return EXIT_SUCCESS;

}

static int initConfigureHackRF(){
	result = hackrf_init();
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_sweep | initConfigureHackRF() | hackrf_init() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_open_by_serial(serial_number, &device);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_sweep | initConfigureHackRF() | hackrf_open() failed: %s (%d)\n", hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	printf("hackrf_sweep | initConfigHackRF() | Success\n");
	return result;
}

static int openFile(){

	if((path == NULL) || (strcmp(path, "-") == 0)) {
		outfile = stdout;
	} else {
		outfile = fopen(path, "wb");
	}

	if(outfile == NULL) {
		fprintf(stderr, "hackrf_sweep | openFile() | Failed to open file: %s\n", path);
		return EXIT_FAILURE;
	}

	printf("hackrf_sweep | openFile() | Success\n");
	return EXIT_SUCCESS;
}

static int setBufOutFile(){
	/* Change outfile buffer to have bigger one to store or read data on/to HDD */
	result = setvbuf(outfile , NULL , _IOFBF , FD_BUFFER_SIZE);
	if( result != 0 ) {
		fprintf(stderr, "hackrf_sweep | setBufOutFile() | setvbuf() failed: %d\n", result);
		usage();
		return EXIT_FAILURE;
	}

	printf("hackrf_sweep | setBufOutFile() | Success\n");
	return result;
}

static int setHackRFParams(){
	printf("hackrf_sweep | setHackRFParams() | call hackrf_sample_rate_set(%.03f MHz)\n",
		   ((float)sampleRate/(float)FREQ_ONE_MHZ));
	result = hackrf_set_sample_rate_manual(device, sampleRate, 1);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_sweep | setHackRFParams() | hackrf_sample_rate_set() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	printf("setHackRFParams | call hackrf_baseband_filter_bandwidth_set(%.03f MHz)\n",
			((float)DEFAULT_BASEBAND_FILTER_BANDWIDTH/(float)FREQ_ONE_MHZ));
	result = hackrf_set_baseband_filter_bandwidth(device, DEFAULT_BASEBAND_FILTER_BANDWIDTH);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "setHackRFParams | hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		usage();
		return EXIT_FAILURE;
	}

	result = hackrf_set_vga_gain(device, vga_gain);
	result |= hackrf_set_lna_gain(device, lna_gain);

	printf("hackrf_sweep | setHackRFParams() | Success\n");
	return result;
}

static int setSweeping()
{
	int customTuneStep = sampleRate/FREQ_ONE_MHZ;
	result = hackrf_init_sweep(device, frequencies, num_ranges, BYTES_PER_BLOCK,
			customTuneStep * FREQ_ONE_MHZ, OFFSET, INTERLEAVED);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_sweep | setSweeping() | hackrf_init_sweep() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int sweeping()
{
	result |= hackrf_start_rx_sweep(device, rx_callback, NULL); //rx callback write are the values to save in the img
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_sweep | sweeping() | hackrf_start_rx_sweep() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

static float sweepDuration()
{
	float durationSweep;

	gettimeofday(&t_start, NULL);
	time_prev = t_start;

	printf("hackrf_sweep | sweepDuration\n");

	while((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false)) {
		float time_difference;
		m_sleep(50);

		gettimeofday(&time_now, NULL);
		durationSweep = TimevalDiff(&time_now,&time_prev);
		if (durationSweep >= 1.0f) {
			time_difference = TimevalDiff(&time_now, &t_start);
			sweep_rate = (float)sweep_count / time_difference;
			fprintf(stderr, "%" PRIu64 "hackrf_sweep | sweepDuration | total sweeps completed, %.2f sweeps/second\n",
					sweep_count, sweep_rate);

			if (byte_count == 0) {
				exit_code = EXIT_FAILURE;
				fprintf(stderr, "\n hackrf_sweep | Couldn't transfer any data for one second.\n");
				break;
			}

			byte_count = 0;
			time_prev = time_now;
		}
	}
	return durationSweep;

}

static int checkAvailabilityAmpOption()
{
	if (amp) {
		printf("hackrf_sweep | checkAvailabilityAmpOption | call hackrf_set_amp_enable(%u)\n", amp_enable);
		result = hackrf_set_amp_enable(device, (uint8_t)amp_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_sweep | checkAvailabilityAmpOption | hackrf_set_amp_enable() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}

		return result;
	}

	return HACKRF_SUCCESS;
}

static int checkAvailabilityAntennaOption(){
	if (antenna) {
		printf("hackrf_sweep | checkAvailabilityAntennaOption | call hackrf_set_antenna_enable(%u)\n", antenna_enable);
		result = hackrf_set_antenna_enable(device, (uint8_t)antenna_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_sweep | checkAvailabilityAntennaOption | hackrf_set_antenna_enable() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}

		return result;
	}

	return HACKRF_SUCCESS;
}

static void checkStreaming()
{
	result = hackrf_is_streaming(device);	
	
	if(do_exit) 
	{
		fprintf(stderr, "\nhackrf_sweep | Exiting...\n");
	}
	
	else 
	{
		fprintf(stderr, "\n hackrf_sweep | Exiting... hackrf_is_streaming() result: %s (%d)\n",
			   hackrf_error_name(result), result);
	}

}

static int endConnection(){
	if(device != NULL) 
	{
		result = hackrf_close(device);
		if(result != HACKRF_SUCCESS) 
		{
			fprintf(stderr, "endConnection | hackrf_close() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
				   return EXIT_FAILURE;
		} 
		
		else 
		{
			printf("endConnection | hackrf_close() done\n");
		}
		
		hackrf_exit();
		printf("endConnection | hackrf_exit() done\n");
	}

	fflush(outfile);
	if ( ( outfile != NULL ) && ( outfile != stdout ) ) {
		fclose(outfile);
		outfile = NULL;
		printf("hackrf_sweep | endConnection | fclose() done\n");
	}

	printf("hackrf_sweep | endConnection | Connection close with hackrf success\n");
	return EXIT_SUCCESS;
}

static int reconfigureHackRF()
{
	//gettimeofday(&time_preConfig, NULL);
	
	if(endConnection() == EXIT_FAILURE){ return EXIT_FAILURE; }
	
	if(initConfigureHackRF() == EXIT_FAILURE){ return EXIT_FAILURE; }
	
	if(setHackRFParams() == EXIT_FAILURE){ return EXIT_FAILURE; }

	if(setSweeping() == EXIT_FAILURE){ return EXIT_FAILURE; }

	//gettimeofday(&time_postConfig, NULL);

	//printf("Time during reconfiguring hackRF: %f\n", TimevalDiff(&time_postConfig, &time_preConfig));
	return EXIT_SUCCESS;
}

static void freeFFTMemory()
{
	fftwf_free(fftwIn);
	fftwf_free(fftwOut);
	fftwf_free(pwr);
	fftwf_free(window);
	fftwf_free(ifftwIn);
	fftwf_free(ifftwOut);
}

/*****************/
/**
 * MAIN
 */
int main(int argc, char** argv) 
{
	int opt = 0, i, nElements;

	showMenu(opt, argc, argv);

	if(checkParams() == EXIT_FAILURE){ return EXIT_FAILURE; } 
	while((fftSize +4) % 8){ fftSize++; }

	fft_bin_width = (double)sampleRate / fftSize;
	fftwIn = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize);
	fftwOut = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fftSize);
	fftwPlan = fftwf_plan_dft_1d(fftSize, fftwIn, fftwOut, FFTW_FORWARD, FFTW_MEASURE);
	pwr = (float*)fftwf_malloc(sizeof(float) * fftSize);
	window = (float*)fftwf_malloc(sizeof(float) * fftSize);
	for (i = 0; i < fftSize; i++) {
		window[i] = (float) (0.5f * (1.0f - cos(2 * M_PI * i / (fftSize - 1))));
	}

#ifdef _MSC_VER
	if(binary_output) {
		_setmode(_fileno(stdout), _O_BINARY);
	}
#endif

	if(initConfigureHackRF() == EXIT_FAILURE){ return EXIT_FAILURE; }

	if(strstr(pathFits, "fits") != NULL) // AQUI
	{	
		if(openFile() == EXIT_FAILURE){ return EXIT_FAILURE; }

		if(setBufOutFile() == EXIT_FAILURE){ return EXIT_FAILURE; }
	}

#ifdef _MSC_VER
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
#else
	signal(SIGINT, &sigint_callback_handler);
	signal(SIGILL, &sigint_callback_handler);
	signal(SIGFPE, &sigint_callback_handler);
	signal(SIGSEGV, &sigint_callback_handler);
	signal(SIGTERM, &sigint_callback_handler);
	signal(SIGABRT, &sigint_callback_handler);
#endif

	int customTuneUp = sampleRate/FREQ_ONE_MHZ;
	if(setHackRFParams() == EXIT_FAILURE){ return EXIT_FAILURE; }

	for(i = 0; i < num_ranges; i++) {
		step_count = 1 + (frequencies[2*i+1] - frequencies[2*i] - 1)
				/ customTuneUp;
		frequencies[2*i+1] = (uint16_t) (frequencies[2*i] + step_count * customTuneUp);
		fprintf(stderr, "hackrf_sweep | Sweeping from %u MHz to %u MHz\n",
				frequencies[2*i], frequencies[2*i+1]);
	}

	printf("hackrf_sweep | HackRF One configuration DONE.\n");

	nElements = naxes[0]*naxes[1];
	//samples = (float*)calloc(nElements,sizeof(float));
	//if(samples == NULL)
	//{
	//	printf("hackrf_sweep | Samples not allocated in memory.\n");
	//	exit(0);
	//}

	printf("hackrf_sweep | calling configuration timer\n");
	setTimerParams();

	if (setSweeping() == EXIT_FAILURE) { return EXIT_FAILURE; }

	printf("hackrf_sweep | Start triggering %d times\n", TRIGGERING_TIMES);

	printf("hackrf_sweep | ===SWEEPING STARTED===\n");

    //Run timer for this iteration 
	totalDuration = hackRFTrigger();

	if (sweeping() == EXIT_FAILURE) { return EXIT_FAILURE; }

	printf("hackrf_sweep | ===SWEEPING DONE===\n");

	if (checkAvailabilityAmpOption() == EXIT_FAILURE || checkAvailabilityAntennaOption() == EXIT_FAILURE){ return EXIT_FAILURE; }
	
	//fprintf(stderr, "Total sweep completed successfully: %d out of %d", counterSucess, TRIGGERING_TIMES);
	
	durationSweeps += sweepDuration();

	checkStreaming();   	
    
	printf("hackrf_sweep | All triggering actions(%d) where completed\n", TRIGGERING_TIMES);
    printf("Iterations: %d ||",TRIGGERING_TIMES);
	printf("\tTotal duration: %.2f s ||",totalDuration);
	printf("\t Total sweep time : %.2f s ||",durationSweeps);
	printf("\t sweepingTime/totalDuration: %.2f%%\n", 100*durationSweeps/totalDuration);
	

	/*if(strstr(path,"fits") != NULL)
	{
		fflush(outfile);
	}*/

	if(endConnection() == EXIT_FAILURE){ return EXIT_FAILURE; }
/*	
	for (i = 0; i< nElements; i++)
	{
		printf("%\n",samples[i]);
		
	}*/
/*	if(strstr(pathFits,"fits")!=NULL)
	{
		generateFitsFile(pathFits, samples);
	}
*/
	
	freeFFTMemory();
	free(samples);
	printf("hackrf_sweep | The dynamic memory used was successfully released.\nEND.\n");

	return exit_code;
	
}
