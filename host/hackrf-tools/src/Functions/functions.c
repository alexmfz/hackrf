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
#define FFTSIZE_STD	(14400)
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
extern char* path;
extern char pathFits[];
extern unsigned int vga_gain;
extern unsigned int lna_gain;
extern uint32_t requested_fft_bin_width;
extern int fftSize;
extern bool one_shot;
extern bool finite_mode;
extern bool binary_output;
extern bool ifft_output;
extern int sampleRate;
uint32_t nChannels = 200;

/*******/

/**
 * Modos de uso de la app. Ejemplo: ./hackrf_sweep f45:870 -w50000 -1 -r a.out
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
	fprintf(stderr, "\t[-B] # binary output\n");
	fprintf(stderr, "\t[-I] # binary inverse FFT output\n");
	fprintf(stderr, "\t-r filename # output file\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Output fields:\n");
	fprintf(stderr, "\tdate, time, hz_low, hz_high, hz_bin_width, num_samples, dB, dB, . . .\n");
	printf("functions | Usage\n");
}

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


/*Parameters for fits file*/
int assignFitsParameters()
{
	printf("functions | assignFitsParameters | Filaname: %s\n",pathFits);
	if(strstr(pathFits, "fits") != NULL)
	{
		naxes[0] = TOTAL_SAMPLES_PER_FREQUENCY; //3600
		naxes[1] = numberOfSteps; //200
		//naxes[3] = naxes[0] * naxes[1];

		if(naxes[1] <=2)
		{
			perror("functions | assignFitsParameters | Number of steps cannot be less than 0");
			return EXIT_FAILURE;
		}

		if(naxes[0] <= 0)
		{
			perror("functions | assignFitsParameters | Number of samples cannot be less than 0");
			return EXIT_FAILURE;
		}

		/*Creating Fits File*/
		printf("functions | Receiving measures and saving as a FITS file\n\n||===Parameters FITS file===||\n");
		printf("functions | assignFitsParameters | Freq min: %d\n", freq_min);
		printf("functions | assignFitsParameters | Freq max: %d\n", freq_max);
		printf("functions | assignFitsParameters | Step Value: %lf\n",step_value);
		printf("functions | assignFitsParameters | Number of steps(X): %ld\n",naxes[0]);
		printf("functions | assignFitsParameters | Number of samples(Z) per frequency: %d\n", fftSize/4);
		printf("functions | assignFitsParameters | Number of total samples : %ld\n", naxes[0]*naxes[1]);
		
		if(pathFits==NULL )
		{
			perror("functions | assignFitsParameters | File not able");
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/**
 * Asigna los parametros genericos de la API
 * 
 */
void assignGenericParameters()
{	
	numberOfSteps = nChannels;

	step_value = (float)(freq_max - freq_min)/numberOfSteps;
	requested_fft_bin_width = step_value*FREQ_ONE_MHZ;

	sampleRate = CUSTOM_SAMPLE_RATE_HZ;

	fftSize = sampleRate/requested_fft_bin_width;

	printf("functions | assignGenericParameters | Step Value: %f MHz\n", step_value);
	printf("functions | assignGenericParameters | Number of Steps (channels): %d\n", numberOfSteps);
	printf("functions | assignGenericParameters | Sample Rate: %d MHz\n", sampleRate);
	printf("functions | assignGenericParameters | FFT size: %d\n", fftSize);
	printf("functions | assignGenericParameters | Samples per channel:\n %d", fftSize/4);
}

/**
 * Menu of the API
 */
int showMenu(int opt, int argc, char**argv)
{
while( (opt = getopt(argc, argv, "a:f:p:l:g:d:n:N:w:i:1BIr:h?")) != EOF ) {
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

		case 'B':
			binary_output = true;
			break;

		case 'I':
			ifft_output = true;
			break;

		case 'r':
			path = optarg;
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

}
