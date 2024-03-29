#include "../../../libhackrf/src/hackrf.h"
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

int minData(int * samples);
int maxData(int * samples);
long getSecondsOfDayOfFirstSweeping(struct tm timeFirstSweeping);
int createFile(char filename[]);
int createImage();
void updateHeadersFitsFileImage(struct tm localTimeFirst, struct tm localTimeLast, uint32_t freq_min);
int insertDataImage(int* samples);
int createBinTable();
void updateHeadersFitsFileBinTable();
void closeFits();
int generateFitsFile(char fileFitsName[], int*samples, struct tm localTimeFirst, struct tm localTimeLast, uint32_t freq_min);
int saveFrequencies(uint32_t freq_min, uint32_t freq_max, int nRanges, float step_value_between_ranges);
int saveTimes(int i, int triggeringTimes, char* sweepingTime); // TODO: Wont be used by the moment
int saveTimeSteps();
int saveSamples(int i, int powerSample, int nElements); // TODO: Wont be used by the moment
int checkSavedData(int nElements);
int writeHackrfDataIntoTxtFiles(struct tm localTimeFirst, struct tm localTimeLast, int* samplesOrdered, float* frequencies, float* times);