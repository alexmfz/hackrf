/**
 * @file functions.c
 * @author Alejandro
 * @brief 
 * @version 0.1
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
#endif

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

/*EXTERN VARIABLES*/
#define CUSTOM_SAMPLE_RATE_HZ (20000000)
#define FREQ_ONE_MHZ (1000000ull)
#define FREQ_MAX_MHZ (7250) /* 7250 MHz */
#define FFTMAX 	(8180)
#define FFT_DEFAULT_SIZE	(20)

#define TRIGGERING_TIMES (3600) //3600

#define MAX_TIME_MINUTES (15)
#define SAMPLES_PER_S	(4)
#define CONVERSION_MIN_TO_S (60)
#define TOTAL_SAMPLES_PER_FREQUENCY (MAX_TIME_MINUTES*CONVERSION_MIN_TO_S*SAMPLES_PER_S)

extern int numberOfSteps;
extern long naxes[2];
extern float step_value;

extern const char*serial_number;
extern bool amp;
extern uint32_t amp_enable;
extern bool antenna;
extern uint32_t antenna_enable;
extern uint32_t freq_min;
extern uint32_t freq_max;
extern uint16_t frequencies[MAX_SWEEP_RANGES*2];

extern uint32_t num_sweeps;
extern int num_ranges;
extern int result;
 extern char pathFits[];
extern unsigned int vga_gain;
extern unsigned int lna_gain;
extern uint32_t requested_fft_bin_width;
extern int fftSize;
extern bool one_shot;
extern bool finite_mode;
extern float sampleRate;

uint32_t nChannels = 200;

/*******/

/**
 * @brief  Shows how to use the API
 * @note   
 * @retval None
 */
void usage() {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t[-h] # this help\n");
	fprintf(stderr, "\t[-d serial_number] # Serial number of desired HackRF\n");
	fprintf(stderr, "\t[-a amp_enable] # RX RF amplifier 1=Enable, 0=Disable\n");
	fprintf(stderr, "\t[-f freq_min:freq_max] # minimum and maximum frequencies in MHz\n");
	fprintf(stderr, "\t[-n number channels] # Number of channels, default value = 200");
	fprintf(stderr, "\t[-p antenna_enable] # Antenna port power, 1=Enable, 0=Disable\n");
	fprintf(stderr, "\t[-l gain_db] # RX LNA (IF) gain, 0-40dB, 8dB steps\n");
	fprintf(stderr, "\t[-g gain_db] # RX VGA (baseband) gain, 0-62dB, 2dB steps\n");
	fprintf(stderr, "\t[-w bin_width] # FFT bin width (frequency resolution) in Hz, 2445-5000000\n");
	fprintf(stderr, "\t[-1] # one shot mode\n");
	fprintf(stderr, "\t[-N num_sweeps] # Number of sweeps to perform\n");
	fprintf(stderr, "\t-r filename # output file\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Output fields:\n");
	fprintf(stderr, "\tdate, time, hz_low, hz_high, hz_bin_width, num_samples, dB, dB, . . .\n");
	printf("functions | Usage\n");
}

/**
 * @brief  Parse uint32 data
 * @note   
 * @param  s: sring expected
 * @param  value: value expected
 * @retval Returns the result of the operation (HACKRF_SUCCESS => OK ; HACKRF_ERROR_INVALID_PARAM => WRONG)
 */
int parse_u32(char* s, uint32_t* const value) {
	uint_fast8_t base = 10;
	char* s_end;
	uint64_t ulong_value;

	if( strlen(s) > 2 ) {
		if( s[0] == '0' ) {
			if( (s[1] == 'x') || (s[1] == 'X') ) {
				base = 16;
				s += 2;
			} else if( (s[1] == 'b') || (s[1] == 'B') ) {
				base = 2;
				s += 2;
			}
		}
	}

	s_end = s;
	ulong_value = strtoul(s, &s_end, base);
	if( (s != s_end) && (*s_end == 0) ) {
		*value = (uint32_t)ulong_value;
		return HACKRF_SUCCESS;
	} else {
		return HACKRF_ERROR_INVALID_PARAM;
	}
}

/**
 * @brief  Parses the frequency range
 * @note   
 * @param  s: string expected as parameter
 * @param  value_min: min value of the range
 * @param  value_max: max value of the range
 * @retval Returns the result of the operation (HACKRF_SUCCESS => OK ; HACKRF_ERROR_INVALID_PARAM => WRONG)
 */
int parse_u32_range(char* s, uint32_t* const value_min, uint32_t* const value_max) {
	int result;

	char *sep = strchr(s, ':');
	if (!sep)
		return HACKRF_ERROR_INVALID_PARAM;

	*sep = 0;

	result = parse_u32(s, value_min);
	if (result != HACKRF_SUCCESS)
		return result;
	result = parse_u32(sep + 1, value_max);
	if (result != HACKRF_SUCCESS)
		return result;

	return HACKRF_SUCCESS;
}

/**
 * @brief  Generate a fits name in a dynamic way with this format - hackRFOne_UAH_DDMMYYYY_HH_MM.fits
 * @note   
 * @param baseName: Time when program was executed
 * @retval None
 */
void generateDynamicName(struct tm baseName)
{
    char date[50]="";
    char suffix[50] = ".fits";
    char preffix[50] = "hackRFOne_UAH_";

    strftime(date, sizeof date, "%d%m%Y_%Hh_%Mm", &baseName);
    
	strcat(date, suffix);
	strcat(preffix,date);
	strcpy(pathFits, preffix);
	fprintf(stderr, "Filename: %s\n", pathFits);
}

/**
 * @brief  Assing the parameters of the fits file
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int assignFitsParameters()
{
	printf("functions | assignFitsParameters() | Assigning parameters of fits file\n");
	printf("functions | assignFitsParameters() | Filaname: %s\n",pathFits);

	if(strstr(pathFits, "fits") != NULL)
	{
		naxes[0] = TRIGGERING_TIMES;//TOTAL_SAMPLEST_PER_FREQUENCY; //3600
		naxes[1] = numberOfSteps; //200
		//naxes[3] = naxes[0] * naxes[1];

		if(naxes[1] <=2)
		{
			fprintf(stderr, "functions | assignFitsParameters() | Number of steps cannot be less than 0");
			return EXIT_FAILURE;
		}

		if(naxes[0] <= 0)
		{
			fprintf(stderr, "functions | assignFitsParameters() | Number of samples cannot be less than 0");
			return EXIT_FAILURE;
		}

		/*Creating Fits File*/
		printf("functions | assignFitsParameters() | Receiving measures and saving as a FITS file\n\n||===Parameters FITS file===||\n");
		printf("functions | assignFitsParameters() | Freq min: %d\n", freq_min);
		printf("functions | assignFitsParameters() | Freq max: %d\n", freq_max);
		printf("functions | assignFitsParameters() | Step Value: %f\n",step_value);
		printf("functions | assignFitsParameters() | Number of steps(X): %ld\n",naxes[0]);
		printf("functions | assignFitsParameters() | Number of samples(Z) per frequency: %d\n", fftSize/4);
		printf("functions | assignFitsParameters() | Number of total samples : %ld\n", naxes[0]*naxes[1]);
		
		if(pathFits==NULL )
		{
			fprintf(stderr, "functions | assignFitsParameters() | File not able\n");
			return EXIT_FAILURE;
		}
	}
	
	printf("functions | assignFitsParameters() | Execution Sucess\n");
	return EXIT_SUCCESS;
}

/**
 * @brief  Assign to variables the values of generic parameters of the API to custome it
 * @note   
 * @retval None
 */
void assignGenericParameters()
{
	printf("functions | assignGenericParameters() | Assigning parameters of API\n ===GENERIC PARAMS===\n");
	
	numberOfSteps = nChannels;
	fftSize = FFT_DEFAULT_SIZE;

	step_value = (float) (freq_max - freq_min)/numberOfSteps;
	requested_fft_bin_width = step_value*FREQ_ONE_MHZ;

	sampleRate = fftSize*requested_fft_bin_width;
	//sampleRate = CUSTOM_SAMPLE_RATE_HZ;

//	fftSize = sampleRate/requested_fft_bin_width;

	printf("functions | assignGenericParameters() | Number of Channels: %d\n", nChannels);
	printf("functions | assignGenericParameters() | Step Value: %f MHz\n", step_value);
	printf("functions | assignGenericParameters() | Sample Rate: %f MHz\n", sampleRate);
	printf("functions | assignGenericParameters() | FFT size: %d\n", fftSize);	
	printf("functions | assignGenericParameters() | Samples per channel: %d\n", fftSize/4);

	printf("functions | assignGenericParameters() | Execution Success\n");
}

/**
 * @brief  Basic configuration of the API that will assign values dependly of argumentsa
 * @note   
 * @param  opt: possible options
 * @param  argc: argument
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int execApiBasicConfiguration(int opt, int argc, char**argv)
{
while( (opt = getopt(argc, argv, "a:f:p:l:g:d:n:N:w:i:1r:h?")) != EOF ) {
		result = HACKRF_SUCCESS;
		switch( opt ) 
		{
		case 'd':
			serial_number = optarg;
			break;

		case 'a':
			amp = true;
			result = parse_u32(optarg, &amp_enable);
			break;
		case 'n':
			result = parse_u32(optarg, &nChannels);
			break;

		case 'f':
			result = parse_u32_range(optarg, &freq_min, &freq_max);
			if(freq_min >= freq_max) {
				fprintf(stderr,
						"functions | showMenu | argument error: freq_max must be greater than freq_min.\n");
				usage();
				return EXIT_FAILURE;
			}
			if(FREQ_MAX_MHZ <freq_max) {
				fprintf(stderr,
						"functions | showMenu | argument error: freq_max may not be higher than %u.\n",
						FREQ_MAX_MHZ);
				usage();
				return EXIT_FAILURE;
			}
			if(MAX_SWEEP_RANGES <= num_ranges) {
				fprintf(stderr,
						"functions | showMenu | argument error: specify a maximum of %u frequency ranges.\n",
						MAX_SWEEP_RANGES);
				usage();
				return EXIT_FAILURE;
			}
			frequencies[2*num_ranges] = (uint16_t)freq_min;
			frequencies[2*num_ranges+1] = (uint16_t)freq_max;
			num_ranges++;
			break;

		case 'p':
			antenna = true;
			result = parse_u32(optarg, &antenna_enable);
			break;

		case 'l':
			result = parse_u32(optarg, &lna_gain);
			break;

		case 'g':
			result = parse_u32(optarg, &vga_gain);
			break;

		case 'N':
			finite_mode = true;
			result = parse_u32(optarg, &num_sweeps);
			break;

		case 'w':
			result = parse_u32(optarg, &requested_fft_bin_width);
			fftSize = CUSTOM_SAMPLE_RATE_HZ / requested_fft_bin_width; //Number of samples per frecuency/4
			break;

		case '1':
			one_shot = true;
			break;

		case 'r':
			//path = optarg;
			strcpy(pathFits,optarg);
			break;

		case 'h':
		case '?':
			usage();
			return EXIT_SUCCESS;

		default:
			fprintf(stderr, "functions | showMenu | unknown argument '-%c %s'\n", opt, optarg);
			usage();
			return EXIT_FAILURE;
		}
		
		if( result != HACKRF_SUCCESS ) {
			fprintf(stderr, "functions | showMenu | argument error: '-%c %s' %s (%d)\n", opt, optarg, hackrf_error_name(result), result);
			usage();
			return EXIT_FAILURE;
		}		
	}

	assignGenericParameters();
	assignFitsParameters();
	return EXIT_SUCCESS;
}

/**
 * @brief  Wait until the time is multiple of 15 minutes and 0 seconds to execute sweepings
 * @note   
 * @retval Return 1 when time matches with the conditions of start XX:MM/15:00
 */
void startExecution()
{
    time_t tStart;
    struct tm tmStart;
    char startString[50];
    tStart = time(NULL);
    localtime_r(&tStart, &tmStart);
    
    tmStart.tm_min = tmStart.tm_min + 15 - tmStart.tm_min%15;
    tmStart.tm_sec = 0;
    strftime(startString, sizeof(startString), "%H:%M:%S", &tmStart);
    
    tStart = time(NULL);
    localtime_r(&tStart, &tmStart);
    
    printf("functions| startExecution() | Execution will start at %s\n", startString);

    while(tmStart.tm_min %15 != 0 || tmStart.tm_sec != 0 )
    {
        tStart = time(NULL);
        localtime_r(&tStart, &tmStart);
    }

    strftime(startString, sizeof(startString), "%H:%M:%S", &tmStart);
    printf("functions| startExecution() | Execution Started\n");
}