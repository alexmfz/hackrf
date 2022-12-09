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

long minData(float * samples);
long maxData(float * samples);
long getSecondsOfDayOfFirstSweeping(struct tm timeFirstSweeping);
int create(char filename[]);
void updateHeadersFitsFile( char startDate[], char timeStart[], char endDate[], char timeEnd[], uint32_t freq_min, struct tm timeFirstSweeping);
int insertData(float* samples);
void closeFits();
int generateFitsFile(char filename[], float* samples, uint32_t freq_min, uint32_t freq_max, float step_value, char startDate[], char timeStart[], char endDate[], char timeEnd[], struct tm timeFirstSweeping);
int saveFrequencies(uint32_t freq_min, uint32_t freq_max, int nRanges, float step_value_between_ranges);
int saveTimes(int i, int triggeringTimes, char* sweepingTime); // TODO: Wont be used by the moment
int saveSamples(int i, float powerSample, int nElements); // TODO: Wont be used by the moment
int checkSavedData(int nElements);
void associateFreqsToSample(); // TODO: Still not develop
