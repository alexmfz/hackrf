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

void usage();
int assignFitsParameters();
int showMenu(int opt, int argc, char**argv);
int parse_u32(char* s, uint32_t* const value);
int parse_u32_range(char* s, uint32_t* const value_min, uint32_t* const value_max);
void assignGenericParameters();
