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


int create(char filename[]);
void updateHeadersFitsFile( char startDate[], char timeStart[], char endDate[], char timeEnd[]);
int insertData(float* samples);
void closeFits();
int generateFitsFile(char filename[], float* samples, uint32_t freq_min, uint32_t freq_max, float step_value, char startDate[], char timeStart[], char endDate[], char timeEnd[]);
int saveFrequencies(uint32_t freq_min, uint32_t freq_max, float step_value);
int saveTimes(int i, int triggeringTimes, char* sweepingTime); // TODO: Wont be used by the moment
int saveSamples(int i, float powerSample, int nElements); // TODO: Wont be used by the moment
int checkSavedData(int nElements);
void associateFreqsToSample(); // TODO: Still not develop
