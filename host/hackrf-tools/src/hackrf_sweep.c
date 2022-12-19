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

#define FREQ_ONE_MHZ (1000000ull)
#define FREQ_MIN_MHZ (0)    /*    0 MHz */
#define FREQ_MAX_MHZ (7250) /* 7250 MHz */

#define FFTMAX 	(8180)
#define FFTMIN 	(4)
#define CUSTOM_SAMPLE_RATE_HZ (20000000)
#define TRIGGERING_TIMES (3600) //3600
#define DEFAULT_BASEBAND_FILTER_BANDWIDTH (15000000) /* 15MHz default */

#define TUNE_STEP (CUSTOM_SAMPLE_RATE_HZ / FREQ_ONE_MHZ)
#define OFFSET 7500000

#define BLOCKS_PER_TRANSFER 16
#define THROWAWAY_BLOCKS 2

#if defined _WIN32
	#define m_sleep(a) S0.000050sleep( (a) )
#else
	#define m_sleep(a) usleep((a*1000))
#endif

/********************/

/*** My variables ***/
int result = 0; // Result operation code of hackrf functions: EXIT_SUCCESS | EXIT FAILURE or  HACKRF_SUCCESS | HACKRF_ERROR
float totalDuration = 0; // Total duration (should be aroung 15 minutes)
bool success = false; // It determines if a iteration was successfull or not (frue => OK, false ==> Error)
int counterSucess = 0; // It determines the number of iterations successfull
int flag_initialFreqCaught = 0; // Flag to check if first frecuency (freq_min) was caught (0 => not capture, 1 => captured)
int row_id = 0; //Max value it should be nRanges

float step_value = 0; // (freqmax-freqmin)/nChannels (MHz) ==> Channel bandwidth
int numberOfSteps = 0; // Number of channels

float sampleRate = 0; // Custom sample rate 

char pathFits[50]; // File name of fits file
extern long naxes[2]; // Number of axis of fits file

extern float *frequencyDataRanges; // Frecuency Datas of the sweeping in ranges
extern float *frequencyDatas; // Frecuency Datas of the sweeping in steps
extern float *timeSteps; // Time data in steps

extern float* samples; // Array of float samples where dbs measures will be saved (but are disordered)
float * samplesOrdered; // Array of float samples where dbs measures will be saved in a correct order
int *flagsOrder; // Array which represents the flags to check if a range of frequency is ordered or not
int * real_order_frequencies; // Insertion order of freqs
int orderValue = 1;  // 1 => Ordered ; 0 => disordered
double previousInsertedFrequency = 0;

extern char timeDatas[TRIGGERING_TIMES][60]; // Time Datas of the sweeping | 3600 dates
int id_sample = 0; // Id samples
int nElements; // Number elements of the fits file
int nRanges = 0; // Number of frequency ranges

int timerFlag = 0; // Timer flag to check if was trigger or not (At handler is set to 1 which means that 0.25s had passed)
extern struct itimerval timer; // Timer struct needed to create a timer

struct timeval timeValStartSweeping ,timeValEndSweeping; // When first and last sweeping are done
struct tm tm_timeStartSweeping, tm_timeEndSweeping; // Beginning and end of sweeping

time_t timeBeginningExecution, timeEndExecution; // When Program start and finish execution
time_t t_timeStartConfig, t_timeEndConfig; // When Program start and finish configuration
time_t t_timeStartSweeping, t_timeEndSweeping; // Timing values of sweeping to determine beginning and end (use for fits headers)
time_t t_timeStartGeneration, t_timeEndGeneration; // When Program start and finish FITS generation

float durationSweeps = 0; // Total duration of sweepings ( TODO: Change where it is measured)

/********************/

/*** Predefined variables ***/
uint32_t num_sweeps = 0; // Numbers of sweeps to do (successfull or not)
int num_ranges = 0; // Number od ranges of the frequency (should be 2 always)
uint16_t frequencies[MAX_SWEEP_RANGES*2]; // Array of frecuencies
int step_count; // Step value
unsigned int lna_gain=16, vga_gain=20; // Gains 
uint32_t freq_min = 0; // Predefined min frequency if is not passed by argument
uint32_t freq_max = 6000; // Predefined max frequency if is not passed by argument

uint32_t byte_count = 0; // Bytes transmitted
volatile uint64_t sweep_count = 0; // Number of sweeps done (sucessfull or not)

struct timeval time_start; // Reference time to check beggining of sweep
struct timeval t_start; // Idem
struct timeval time_now; // ""
struct timeval time_prev; // ""

float sweep_rate = 0; // sweep rate which is sweep_count / time_difference

bool amp = false; // amp parameter
uint32_t amp_enable; // amp enabler

bool antenna = false; // antenna parameter
uint32_t antenna_enable; // antenna enabler

bool one_shot = false; // One shot mode (false => Down ; true => Up) set with -1 argument
bool finite_mode = false; // Finite mode (false => Down ; true => Up) set if not -1 argument is given
bool sweep_started = false; // Flag to check that sweeping has already started

int fftSize = 20; // Size of a fft window (Max value = 8180 ; Min value = 4)
double fft_bin_width; // Bandwidth fft 

fftwf_complex *fftwIn = NULL; // FFT data
fftwf_complex *fftwOut = NULL; // FFT data
fftwf_plan fftwPlan = NULL; // FFT data

float* pwr; // Power measures
float* window;
hackrf_device* device = NULL; // Struct of device
const char*serial_number = NULL; // Serial number of hackrf device

uint32_t requested_fft_bin_width = 0;// Argument to set bandwidth (but is in this case configured to not give argument's use)

bool do_exit = false; // It determines if sweeping end or not ( true => Stop sweeping ; false => Keep sweeping)

/********************/

/** Signal Handler**/
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

/**
 * @brief  Calculates difference between 2 times
 * @note   
 * @param  *a: last time 
 * @param  *b: first time
 * @retval difference time
 */
static float TimevalDiff(const struct timeval *a, const struct timeval *b) 
{
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

/**
 * @brief  Set timerFlag to 1 when signal control is captured
 * @note   
 * @param  sig: Signal to be control
 * @retval None
 */
void timerHandler(int sig)
{
    timerFlag = 1;
} 

/**
 * @brief  Starts a timer which will be thrown periodically
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int hackRFTrigger()
{ 
	printf("hackrf_sweep | hackRFTrigger() | Triggering Start\n");
    if(signal(SIGALRM, timerHandler) == SIG_ERR)
    {
        fprintf(stderr, "timer | hackRFTrigger | Unable to catch alarm signal");
		return EXIT_FAILURE;
	}
   
    if(setitimer(ITIMER_REAL, &timer, 0) == -1)
    {
        fprintf(stderr, "timer | hackRFTrigger | Error calling timer");
        return EXIT_FAILURE;
    }

	printf("hackrf_sweep | hackRFTrigger() | Execution Success\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Check if the insertion of frequencies at sweeping is correct or not
 * @note   
 * @param  list_frequencies:	Array of frequencies
 * @param  real_order_frequencies: Real order of this frequencies at sweeping
 * @param  freq_min: 
 * @param  nRanges: number of ranges of frequencies (nChannels/stepValue)
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int checkOrderFreqs(int nRanges, double insertedFrequency) // WIll be invoke at hackrf_sweep callback
{
    int i = 0;
	//printf("Inserted frequency: %f\n", insertedFrequency);
	if (previousInsertedFrequency == 0)
	{
    	printf("\nhackrf_sweep | checkOrderFreqs() | Ordering insertion of frequencies\n");
	}

    if (frequencyDataRanges == NULL || real_order_frequencies == NULL)
    {
        fprintf(stderr, "hackrf_sweep | checkOrderFreqs() | Not reserved memory correctly\n");
        return EXIT_FAILURE;
    }

	if (previousInsertedFrequency == insertedFrequency)
	{
		fprintf(stderr, "hackrf_sweep | checkOrderFreqs() | Frequency checked previously\n");
		return EXIT_SUCCESS;
	}

	previousInsertedFrequency = insertedFrequency;
    for (i = 0; i < nRanges; i++)
    {
        if (frequencyDataRanges[i] == (float)insertedFrequency)
        {
            real_order_frequencies[i] = orderValue;
            orderValue++;
            break;
        }
    }
    
	if ((uint32_t)insertedFrequency == freq_max)
	{
    	printf("hackrf_sweep | checkOrderFreqs() | Execution Success\n");
	}

    return EXIT_SUCCESS;
}

/**
 * @brief  Converts a power sample value into logarithmic value
 * @note   
 * @param  in: Input value
 * @param  scale: Reescale value
 * @retval  Value in log format
 */
float logPower(fftwf_complex in, float scale)
{
	float re = in[0] * scale;
	float im = in[1] * scale;
	float magsq = re * re + im * im;
	return (float) (log2(magsq) * 10.0f / log2(10.0f));
}

/**
 * @brief  Callback function which sweep n times between freq_min and freq_max
 * @note   
 * @param  transfer: Struct where data is being transfered
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int rx_callback(hackrf_transfer* transfer) {
	int8_t* buf;
	uint8_t* ubuf;
	uint64_t frequency; /* in Hz */
	int i, j;
	struct tm *fft_time;
	char time_str[60];
	struct timeval usb_transfer_time;

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
		if (counterSucess == 0)
		{
			t_timeStartSweeping = time(NULL);
			localtime_r(&t_timeStartSweeping, &tm_timeStartSweeping);
			gettimeofday(&timeValStartSweeping, NULL);
		}
			
		if(strstr(pathFits, "fit")!= NULL && timerFlag == 1 && ( frequency == (uint64_t)(FREQ_ONE_MHZ*frequencies[0]) || flag_initialFreqCaught == 1)) 
		{	
			time_t time_stamp_seconds = usb_transfer_time.tv_sec;
			fft_time = localtime(&time_stamp_seconds);
			strftime(time_str, 60, "%Y-%m-%d, %H:%M:%S", fft_time);
			if ( frequency == (uint64_t)(FREQ_ONE_MHZ*frequencies[0])) 
			{ 
				char sweepingTime[60];
				char decimalTime[8] = {"."};
				char totalDecimalsTime[8];
				flag_initialFreqCaught = 1; //First time will enter
				fprintf(stderr, "First Frecuency caught. Setting flag to 1\n"); 

				strcpy(sweepingTime, time_str);
				sprintf(totalDecimalsTime, "%06ld", (long int)usb_transfer_time.tv_usec);
				strncat(decimalTime, totalDecimalsTime, 3);
				strncat(sweepingTime, decimalTime, 4);

				//Save times
				strcpy(timeDatas[counterSucess], sweepingTime);
			}

			/*printf("%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
				time_str,
				(long int)usb_transfer_time.tv_usec,
				(uint64_t)(frequency), //First time: 45MhZ
				(uint64_t)(frequency+sampleRate/4), //45MHz + 5MHz
				fft_bin_width,
				fftSize
				);
		*/
			for(i = 0; (fftSize / 4) > i; i++) 
			{
			//	printf(", %.2f", pwr[i + 1 + (fftSize*5)/8]);
		
				// Save power sample
				samples[id_sample] = pwr[i + 1 + (fftSize*5)/8];
				id_sample++;
			}

			row_id ++;
			/*printf(", row_id =%d", row_id);
			printf("\n");
			printf("%s.%06ld, %" PRIu64 ", %" PRIu64 ", %.2f, %u",
					time_str,
					(long int)usb_transfer_time.tv_usec,
					(uint64_t)(frequency+(sampleRate/2)),
					(uint64_t)(frequency+((sampleRate*3)/4)),
					fft_bin_width,			
					fftSize
					);
			*/			
			for(i = 0; (fftSize / 4) > i; i++) 
			{
			//	printf(", %.2f", pwr[i + 1 + (fftSize/8)]);
				
				// Save power sample
				samples[id_sample] = pwr[i + 1 + (fftSize/8)];
				id_sample++;
			}

			row_id ++;
		/*	printf(", row_id =%d", row_id);
			printf("\n");
			*/
			if (counterSucess == 0)
			{
	
				if(checkOrderFreqs(nRanges, ((double)(frequency)/FREQ_ONE_MHZ)) == EXIT_FAILURE) { return EXIT_FAILURE; }
				if(checkOrderFreqs(nRanges, ((double)(frequency) + sampleRate/2)/FREQ_ONE_MHZ) == EXIT_FAILURE) { return EXIT_FAILURE; }
			}

			if ((uint64_t)(frequency+((sampleRate*3)/4)) >= freq_max*FREQ_ONE_MHZ || row_id == nRanges) //Where the sweep finished
			{
				if (counterSucess == 0)
				{
					if(checkOrderFreqs(naxes[1]/(fftSize/4), ((double)(frequency) + sampleRate*3/4)/FREQ_ONE_MHZ) == EXIT_FAILURE) { return EXIT_FAILURE; }
				}

				row_id = 0 ;
				counterSucess++;
				printf("hackrf_sweep | rx_callback() | Data Caught. Iteration %d finished\n", counterSucess);			
				success = false;
				timerFlag = 0; // Flag down which means that data was caught
				flag_initialFreqCaught = 0; // Set variable to finish iteration
				
				if (counterSucess == TRIGGERING_TIMES)
				{
					t_timeEndSweeping = time(NULL);
					localtime_r(&t_timeEndSweeping, &tm_timeEndSweeping);
					gettimeofday(&timeValEndSweeping, NULL);
				}
			}
		}
		
	}
			
	return 0;
}

/**
 * @brief Checks the params. If not possible values EXIT_FAILURE is returned
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int checkParams(){
	printf("hackrf_sweep | checkParams() | Checking if parameters values are correct\n");

	if (lna_gain % 8)
	{
		fprintf(stderr, "hackrf_sweep | checkParams() | warning: lna_gain (-l) must be a multiple of 8\n");
	}

	if (vga_gain % 2)
	{
		fprintf(stderr, "hackrf_sweep | checkParams() | warning: vga_gain (-g) must be a multiple of 2\n");
	}
	
	if( amp ) 
	{
		if( amp_enable > 1 ) 
		{
			fprintf(stderr, "hackrf_sweep | checkParams() | argument error: amp_enable shall be 0 or 1.\n");
			usage();
			return EXIT_FAILURE;
		}
	}

	if (antenna) 
	{
		if (antenna_enable > 1) 
		{
			fprintf(stderr, "hackrf_sweep | checkParams() | argument error: antenna_enable shall be 0 or 1.\n");
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

	/*
	 * The FFT bin width must be no more than a quarter of the sample rate
	 * for interleaved mode. With our fixed sample rate of 20 Msps, that
	 * results in a maximum bin width of 5000000 Hz.
	 */
	if(fftSize < FFTMIN) 
	{
		fprintf(stderr, "hackrf_sweep | checkParams() | argument error: FFT bin width (-w) must be no more than 5000000\n");
		return EXIT_FAILURE;
	}

	if(fftSize > FFTMAX) 
	{
		fprintf(stderr, "hackrf_sweep | checkParams() | argument error: FFT bin width (-w) must be no less than 2445\n");
		return EXIT_FAILURE;
	}

	printf("hackrf_sweep | checkParams() | Execution Success\n");
	return EXIT_SUCCESS;

}

/**
 * @brief  Initialize hackrf and open the serial to able it to communicate
 * 
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int initConfigureHackRF(){
	printf("hackrf_sweep | initConfigHackRF() | Initialize configuration of HackRF One\n");
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

	printf("hackrf_sweep | initConfigHackRF() | Execution Success\n");
	return result;
}

/**
 * @brief  Set HackRF parameters such as sample rate, gains... to start working
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
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

	printf("hackrf_sweep | setHackRFParams() | Execution Success\n");
	return result;
}

/**
 * @brief  Set params of sweeping (frecuencies, number ranges, type of sweeping...)
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int setSweeping()
{
	printf("hackrf_sweep | setSweeping() | Configuring sweeping parameters\n");
	float customTuneStep = (float)sampleRate/(float)FREQ_ONE_MHZ;
	result = hackrf_init_sweep(device, frequencies, num_ranges, BYTES_PER_BLOCK,
			customTuneStep * FREQ_ONE_MHZ, OFFSET, INTERLEAVED);
	if( result != HACKRF_SUCCESS ) {
		fprintf(stderr, "hackrf_sweep | setSweeping() | hackrf_init_sweep() failed: %s (%d)\n",
			   hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}

	printf("hackrf_sweep | setSweeping() | Execution Sucess\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  It invokes callback function to receive data from a hackrf one
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int sweeping()
{
	if (hackRFTrigger() == EXIT_FAILURE) { return EXIT_FAILURE; }

	printf("hackrf_sweep | sweeping() | ===SWEEPING STARTED===\n");
	printf("hackrf_sweep | Start triggering %d times\n", TRIGGERING_TIMES);

	result |= hackrf_start_rx_sweep(device, rx_callback, NULL); //rx callback write are the values to save in the img
	if (result != HACKRF_SUCCESS) {
		fprintf(stderr, "hackrf_sweep | sweeping() | hackrf_start_rx_sweep() failed: %s (%d)\n", hackrf_error_name(result), result);
		return EXIT_FAILURE;
	}
	
	printf("hackrf_sweep | sweeping() | ===SWEEPING DONE===\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  calculates duration of sweeping 
 * @note   TODO: Check it that time is correct
 * @retval duration of sweeping
 */
static float sweepDuration()
{
	float durationSweep;

	gettimeofday(&t_start, NULL);
	time_prev = t_start;

	printf("hackrf_sweep | sweepDuration()\n");

	while((hackrf_is_streaming(device) == HACKRF_TRUE) && (do_exit == false)) {
		float time_difference;
		m_sleep(50);

		gettimeofday(&time_now, NULL);
		durationSweep = TimevalDiff(&time_now,&time_prev);
		if (durationSweep >= 1.0f) {
			time_difference = TimevalDiff(&time_now, &t_start);
			sweep_rate = (float)sweep_count / time_difference;
			/*fprintf(stderr, "%" PRIu64 "hackrf_sweep | sweepDuration() | total sweeps completed, %.2f sweeps/second\n",
					sweep_count, sweep_rate);*/

			if (byte_count == 0) {
				fprintf(stderr, "\n hackrf_sweep | sweepDuration() | Couldn't transfer any data for one second.\n");
				break;
			}

			byte_count = 0;
			time_prev = time_now;
		}
	}
	return durationSweep;

}

/** 
 * @brief  Checks if amp option is enable to activate it
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int checkAvailabilityAmpOption()
{
	printf("hackrf_sweep | checkAvailabilityAmpOption() | Checking availabilty amp option\n");

	if (amp) 
	{
		printf("hackrf_sweep | checkAvailabilityAmpOption() | call hackrf_set_amp_enable(%u)\n", amp_enable);
		result = hackrf_set_amp_enable(device, (uint8_t)amp_enable);
		if (result != HACKRF_SUCCESS) 
		{
			fprintf(stderr, "hackrf_sweep | checkAvailabilityAmpOption() | hackrf_set_amp_enable() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}

		return result;
	}
	
	printf("hackrf_sweep | checkAvailabilityAmpOption() | Execution Success\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Checks if antenna option is enabled to activate it
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int checkAvailabilityAntennaOption()
{
	printf("hackrf_sweep | checkAvailabilityAntennaOption() | Checking availability of antenna option\n");
	if (antenna) 
	{
		printf("hackrf_sweep | checkAvailabilityAntennaOption() | call hackrf_set_antenna_enable(%u)\n", antenna_enable);
		result = hackrf_set_antenna_enable(device, (uint8_t)antenna_enable);
		if (result != HACKRF_SUCCESS) {
			fprintf(stderr, "hackrf_sweep | checkAvailabilityAntennaOption() | hackrf_set_antenna_enable() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}

		return result;
	}

	printf("hackrf_sweep | checkAvailabilityAntennaOption() | Execution Success\n");
	return HACKRF_SUCCESS;
}

/**
 * @brief  Check if hackrf one is still streaming
 * @note   
 * @retval None
 */
static void checkStreaming()
{
	printf("hackrf_sweep() | checkStreaming() | Checking streaming status\n");
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

	printf("hackrf_sweep() | checkStreaming() | Execution Success\n");
}

/**
 * @brief  Closes connection with hackrf one and stops receiving data
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int endConnection()
{
	printf("hackrf_sweep | endConnection() | Ending connection with HackRF One\n");
	if(device != NULL) 
	{
		result = hackrf_close(device);
		if(result != HACKRF_SUCCESS) 
		{
			fprintf(stderr, "hackrf_sweep | endConnection() | hackrf_close() failed: %s (%d)\n",
				   hackrf_error_name(result), result);
				   return EXIT_FAILURE;
		} 
		
		else 
		{
			printf("hackrf_sweep | endConnection() | hackrf_close() done\n");
		}
		
		hackrf_exit();
		printf("hackrf_sweep | endConnection() | hackrf_exit() done\n");
	}

	printf("hackrf_sweep | endConnection() | Execution Success\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Reorganize power samples into the correct position
 * @note   
 * @param  ordered_frecuency_position: Correct order of frequency
 * @param  real_order_frequency_position: Real order of frequency at sweeping
 * @param  nRanges: Nº of ranges of frequencies 
 * @param  samples: power samples at sweeping
 * @param  samplesOrdered: power samples organized after sweeping
 * @param  nElements: nElements of total sweepings
 * @param  valuesPerFreq: values per frequency (fftSize/4)
 * @param  nChannels: number of channels 
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int reorganizeSamples(int ordered_frecuency_position, int real_order_frequency_position, int nRanges, float* samples, float* samplesOrdered, int nElements, int valuesPerFreq, long nChannels)
{
    int j = 0, z = 0, i = 0;
    int samplesPerSweeping = valuesPerFreq*nRanges; // 200
    int actualElementIndex = 0; //Actual element index
    float sampleToSave; //sample to backup

    // Initial positions at first iteration
    j = (ordered_frecuency_position-1)*valuesPerFreq;  // Index of ordered samples
    i = j + valuesPerFreq; // actual Index
    z = (real_order_frequency_position-1)*valuesPerFreq; // Index of disordered samples

    while (actualElementIndex < nElements ) // 1000 => nElements
    {
        while(j < i) // valuesPerFreq = fftSize/4
        {
            sampleToSave = samples[j]; // samples[i] at this moment are in incorrect order
            samplesOrdered[j] = samples[z]; // samples[z] are the ones we want to move to the correct order
            samplesOrdered[z] = sampleToSave; // Recover of samples saves as backup
            j++;
            z++;         
        }

        actualElementIndex += samplesPerSweeping; // nRanges of frequencies * valuesPerfrequency index
        j += samplesPerSweeping-valuesPerFreq;
        z += samplesPerSweeping-valuesPerFreq;
        i += samplesPerSweeping;
    }

    flagsOrder[real_order_frequency_position-1] = 1;
    flagsOrder[ordered_frecuency_position-1] = 1; 

    return EXIT_SUCCESS;
}

/**
 * @brief  Free reserved memory for fft data
 * @note   
 * @retval None
 */
static void freeFFTMemory()
{
	fftwf_free(fftwIn);
	fftwf_free(fftwOut);
	fftwf_free(pwr);
	fftwf_free(window);
}

/**
 * @brief  Free reserved memory for fits data
 * @note   
 * @retval None
 */
static void freeFitsMemory()
{
	free(frequencyDataRanges);
	free(frequencyDatas);
	free(timeSteps);
	free(real_order_frequencies);

	free(samples);

	free(flagsOrder);
	free(samplesOrdered);
}

/**
 * @brief Function just to check values
 * @note   
 * @retval None
 */
static void printValuesHackRFOne()
{
	int i = 0;
	int nElements = naxes[0]*naxes[1];
	float step_range = (float)((fft_bin_width)/FREQ_ONE_MHZ)*(fftSize/4);


	/*printf("hackrf_sweep | printValuesHackRFOne() | Data results: Timing\n");
	for (i = 0; i< TRIGGERING_TIMES; i++)
	{
		printf("Time[%d]: %s\n", i, timeDatas[i]);	
	}*/
	
/*	printf("hackrf_sweep | printValuesHackRFOne() | Data results: Power samples\n");
	for (i = 0; i < nElements; i++)
	{
		if (i%5 == 0)
		{
			printf("\n");
		}
		printf("Power sample disordered[%d]: %f\t Power sample ordered[%d]: %f\n", i, samples[i], i, samplesOrdered[i]);
		
	}*/
		
/*	printf("hackrf_sweep | printValuesHackRFOne() | Data results: Frequencies\n");
	for (i = 0; i< nRanges; i++)
	{
		printf("Frequency Ranges[%d]: %f MHz\n", i, frequencyDataRanges[i]);
	}
	*/
/*
		// Print real order frequencies
	for(i = 0; i < nRanges; i++)
    {
        printf("frequency_order[%d]: %d - samples[%d...%d] - Frequency: %f\n",
		i, real_order_frequencies[i],
		(real_order_frequencies[i]-1)*(fftSize/4),
		(real_order_frequencies[i]-1)*(fftSize/4)-1 + (fftSize/4),
		freq_min + (real_order_frequencies[i]-1)*step_range
		);
    }
*/
/*	for(i = 0; i < nRanges; i++)
    {
        printf("flag_order[%d]: %d\n", i, flagsOrder[i]);
    }*/

	/*for (i = 0; i < naxes[1]; i++)
	{
		printf("Frequency[%d]: %.2f\n",i , frequencyDatas[i]);
	}*/
}

/**
 * @brief  Calculate times of the program when is called
 * @note   
 * @param  t_timeStartConfig: Time variable to get the instant
 * @param  tm_timeStartConfig: Time structure to format (beginning and end)
 * @param  timeValStartConfig: Time structure to get the timeof day in secs
 * @retval None
 */
void calculateTimes(time_t* t_time, struct tm* tm_time, struct timeval* timeVal)
{
        gettimeofday(timeVal, NULL);
	    *t_time = time(NULL);
	    localtime_r(t_time, tm_time);
}

/**
 * @brief  Runs all the configuration
 * @note
 * @param opt: option choose at executing
 * @param argc: idem 
 * @param argv: idem
 * @param i: iteration variable  
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int runConfiguration(int argc, char** argv)
{
	int opt = 0, i;
	float step_range = 0;

	printf("=============================================================\n");
	printf("hackrf_sweep | runConfiguration() | Starting Configuration\n");
	if (execApiBasicConfiguration(opt, argc, argv) == EXIT_FAILURE) { return EXIT_FAILURE; }

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


	if(initConfigureHackRF() == EXIT_FAILURE){ return EXIT_FAILURE; }


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

	float customTuneUp = (float)sampleRate/FREQ_ONE_MHZ;
	if(setHackRFParams() == EXIT_FAILURE){ return EXIT_FAILURE; }

	for(i = 0; i < num_ranges; i++) {
		step_count = 1 + (frequencies[2*i+1] - frequencies[2*i] - 1)
				/ customTuneUp;
		frequencies[2*i+1] = (uint16_t) (frequencies[2*i] + step_count * customTuneUp);
		fprintf(stderr, "hackrf_sweep | runConfiguration() | Sweeping from %u MHz to %u MHz\n",
				frequencies[2*i], frequencies[2*i+1]);
	}

	printf("hackrf_sweep | runConfiguration() | HackRF One configuration DONE.\n");

	nElements = naxes[0]*naxes[1];
	step_range = (float)(fft_bin_width/FREQ_ONE_MHZ)*(fftSize/4);
	nRanges = (freq_max - freq_min)/step_range;

	fprintf(stderr, "\nhackrf_sweep | runConfiguration() | nElements = %d\n nRanges = %d\n sampeRate = %.2f\n customTuneUp = %f\n stepValue = %f\n naxes[0] = %ld\n naxes[1] = %ld\n requested_fft_bin_width = %d\n fft_bin_width = %lf\n fftSize/4 = %d\n step_range = %f\n\n",
	 																	nElements, nRanges, sampleRate, customTuneUp, step_value,
																		naxes[0], naxes[1],
																		requested_fft_bin_width, fft_bin_width, fftSize/4, step_range);
	
	flagsOrder = (int*)calloc(nRanges, sizeof(int));
    real_order_frequencies = (int*) calloc(nRanges, sizeof(int)); // positions of frecuencies

	//save frequency data 
    if (saveFrequencies(freq_min, freq_max, nRanges, step_range) == EXIT_FAILURE) { return EXIT_FAILURE; } 
	if (saveTimeSteps() == EXIT_FAILURE) { return EXIT_FAILURE; }

	setTimerParams();

	if (setSweeping() == EXIT_FAILURE) { return EXIT_FAILURE; }

	// Reserve memory for power sample data
    samples = (float*)calloc(nElements,sizeof(float));
	samplesOrdered = (float*)calloc(nElements, sizeof(float));
	
    if (samples == NULL || samplesOrdered == NULL)
    {
        fprintf(stderr, "hackrf_sweep | runConfiguration() | Was not possible to allocate memory for power samples\n");
        return EXIT_FAILURE;
    }

	printf("hackrf_sweep | runConfiguration() | Memory allocated for power samples\n");
	printf("hackrf_sweep | runConfiguration() | Execution Success\n");
	printf("=============================================================\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Runs the functionality
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int runExecution()
{
	printf("=============================================================\n");
	printf("hackrf_sweep | runExecution() | Execution starts\n");

	if (sweeping() == EXIT_FAILURE) { return EXIT_FAILURE; }

	if (checkAvailabilityAmpOption() == EXIT_FAILURE || checkAvailabilityAntennaOption() == EXIT_FAILURE){ return EXIT_FAILURE; }

	durationSweeps += sweepDuration();
	totalDuration = TimevalDiff(&timeValEndSweeping, &timeValStartSweeping);

	checkStreaming();   	
    printf("hackrf_sweep| Total sweep completed successfully: %d out of %d\n", counterSucess, TRIGGERING_TIMES);
	
	printf("hackrf_sweep | All triggering actions(%d) where completed\n", TRIGGERING_TIMES);
	printf("hackrf_sweep | Total duration of sweeping: %.2f s ||", totalDuration);
	
	if(checkSavedData(nElements) == EXIT_FAILURE){ return EXIT_FAILURE; }
	
	if(endConnection() == EXIT_FAILURE){ return EXIT_FAILURE; }

	printf("=============================================================\n");
	printf("hackrf_sweep | runExecution() | Execution Success\n");

	return EXIT_SUCCESS;
}

/**
 * @brief  Runs the generation of fits file
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
static int runGeneration(struct tm localTimeFirst, struct tm localTimeLast)
{
	int i;

	printf("=============================================================\n");
	printf("hackrf_sweep | runGeneration() | Execution starts\n");
	// Copy disordered samples into the ordered one to not check the ordered freqs
	for (i = 0 ; i < nElements; i++)
	{
		samplesOrdered[i] = samples[i];
	}
	
	printf("hackrf_sweep | runGeneration() | reorganizeSamples() | Reorganizing Samples Started\n");
	for (i = 0; i < nRanges; i++)
	{
		if ((i+1) != real_order_frequencies[i] && flagsOrder[i] == 0) //i+1 is the position that should have
		{
			if (reorganizeSamples(i+1, real_order_frequencies[i], nRanges,
								  samples, samplesOrdered,
								  nElements,  fftSize/4, naxes[1]) == EXIT_FAILURE) { return EXIT_FAILURE; }

		}
		
		else
		{
        	flagsOrder[i] = 1;
		}

	}

	for (i = 0; i < nRanges; i++)
	{
		if (flagsOrder[i] ==0 )
		{
			fprintf(stderr, "hackrf_sweep | runGeneration() | reorganizeSamples() | Flag[%d] is down. Reorganization of samples failed\n", i);
			return EXIT_FAILURE;
		}
	}
	
	printf("hackrf_sweep | runGeneration() | reorganizeSamples() | Execution Success;\n");
	if(strstr(pathFits,"fit")==NULL || (generateFitsFile(pathFits,
														  samplesOrdered,
														  localTimeFirst, localTimeLast,
														  freq_min)) == EXIT_FAILURE){ return EXIT_FAILURE; }
	
	printf("=============================================================\n");
	printf("hackrf_sweep | runGeneration() | Execution Success\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Main thread 
 * @note   Execution example: ./hackrf_sweep -f45:245 > test.out
 * @param  argc: 
 * @param  argv: 
 * @retval EXIT_SUCCESS
 */
int main(int argc, char** argv) 
{	
	struct tm tm_timeBeginningExecution, tm_timeEndExecution; // Struct of time values at beginning and end of the program
	struct timeval timeValStartExecution, timeValEndExecution; // Use to get duration of the exection
	char timeStartProgram[70], timeEndProgram[70]; // Time as date and hours
	
	struct tm tm_timeStartConfig, tm_timeEndConfig; // Struct of time values at beginning and end of the configuration
	struct timeval timeValStartConfig, timeValEndConfig; // Use to get duration of the configuration
	char timeStartConfig[70], timeEndConfig[70]; // Time as date and hours

	struct tm tm_timeStartGeneration, tm_timeEndGeneration; // Struct of time values at beginning and end of the fits generation
	struct timeval timeValStartGeneration, timeValEndGeneration; // Use to get duration of the fits generation
	char timeStartGeneration[70], timeEndGeneration[70]; // Time as date and hours

	char timeStartSweeping[70], dateStartSweeping[70];
	char timeEndSweeping[70], dateEndSweeping[70];

	/* START PROGRAM */
	calculateTimes(&timeBeginningExecution, &tm_timeBeginningExecution, &timeValStartExecution);
	strftime(timeStartProgram, sizeof timeStartProgram, "%Y-%m-%d %H:%M:%S", &tm_timeBeginningExecution);

	generateDynamicName(tm_timeBeginningExecution);

	/* START CONFIGURATION */
	calculateTimes(&t_timeStartConfig, &tm_timeStartConfig, &timeValStartConfig);
	strftime(timeStartConfig, sizeof timeStartConfig, "%Y-%m-%d %H:%M:%S", &tm_timeStartConfig);

	if (runConfiguration(argc, argv) == EXIT_FAILURE) { return EXIT_FAILURE; }

	calculateTimes(&t_timeEndConfig, &tm_timeEndConfig, &timeValEndConfig);
	strftime(timeEndConfig, sizeof timeEndConfig, "%Y-%m-%d %H:%M:%S", &tm_timeEndConfig);

	/* END CONFIGURATION*/

	//startExecution();

    /* START EXECUTION */

	if (runExecution() == EXIT_FAILURE) { return EXIT_FAILURE; }

	strftime(timeStartSweeping, sizeof timeStartSweeping, "%Y-%m-%d %H:%M:%S", &tm_timeStartSweeping);
	strftime(dateStartSweeping, sizeof dateStartSweeping, "%Y-%m-%d %H:%M:%S", &tm_timeEndSweeping);

	strftime(dateEndSweeping, sizeof dateEndSweeping,"%Y-%m-%d", &tm_timeEndSweeping);
	strftime(timeEndSweeping, sizeof timeEndSweeping, "%Y-%m-%d %H:%M:%S", &tm_timeEndSweeping);

	/* END EXECUTION */


	/*START GENERATION */
	calculateTimes(&t_timeStartGeneration, &tm_timeStartGeneration, &timeValStartGeneration);
	strftime(timeStartGeneration, sizeof timeStartGeneration, "%Y-%m-%d %H:%M:%S", &tm_timeStartGeneration);

	if (runGeneration(tm_timeStartSweeping, tm_timeEndSweeping) == EXIT_FAILURE) { return EXIT_FAILURE; }

	calculateTimes(&t_timeEndGeneration, &tm_timeEndGeneration, &timeValEndGeneration);
	strftime(timeEndGeneration, sizeof timeEndGeneration, "%Y-%m-%d %H:%M:%S", &tm_timeEndGeneration);
	printValuesHackRFOne();

	/* END GENERATION */

	freeFFTMemory();
	freeFitsMemory();
	printf("hackrf_sweep | The dynamic memory used was successfully released.\nEND.\n");

	/* END PROGRAM */
	calculateTimes(&timeEndExecution, &tm_timeEndExecution, &timeValEndExecution);

	timeEndExecution = time(NULL);
	strftime(timeEndProgram, sizeof timeEndProgram, "%Y-%m-%d %H:%M:%S", &tm_timeEndExecution);

	printf("=============================================================\n");
	fprintf(stderr, "Time parameters hackrf_sweep f%d:%d-%d Iterations\n"
					"Program Execution Start: %s\t Program Execution Finish: %s\t Duration: %fs\n"
					"Configuration Start: %s\t Configuration Finish: %s\t Duration: %fs\n"
					"Sweeping Start: %s\t Sweeping Finish: %s\t Duration: %fs\n" 
					"Generation FITS Part Start: %s\t Generation FITS Part Finish: %s\t Duration: %fs\n"
					"Filename generated: %s\n",
					freq_min, freq_max, TRIGGERING_TIMES,
					timeStartProgram, timeEndProgram, TimevalDiff(&timeValEndExecution, &timeValStartExecution),
					timeStartConfig, timeEndConfig, TimevalDiff(&timeValEndConfig, &timeValStartConfig),
					timeStartSweeping, timeEndSweeping, TimevalDiff(&timeValEndSweeping, &timeValStartSweeping),
					timeStartGeneration, timeEndGeneration, TimevalDiff(&timeValEndGeneration, &timeValStartGeneration),
					pathFits
			);
	return EXIT_SUCCESS;
	
}
