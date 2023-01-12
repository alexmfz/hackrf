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
#include <sys/time.h>

#if defined(__GNUC__)
#include <unistd.h>
#endif
#include <signal.h>
#include <math.h>


static void usage()
{
    fprintf(stderr, "\t[-h] --> Help\n");
    fprintf(stderr, "\t[-t] --> ScheduleTime\n");
}

/**
 * @brief  Checks if the string introduced is well formated or not and cast it into a date
 * @note
 * @param timeScheduled: String received from scheduled.cfg not reformated
 * @param tm_timeScheduled: timeScheduled reformated as date
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
*/

static int formatStringToDate(char* timeScheduled, struct tm* tm_timeScheduled)
{
    char hour[3];
    char min[3];
    char sec[3];
    int len = strlen(timeScheduled);
    char double_dot[2] = ":";

    if (len != 8)
    {
        printf("Error, Incorrect format of date introduced\n");
        return EXIT_FAILURE;
    }    
    
    strncpy(hour, timeScheduled, 2);
    
    if (strncmp(double_dot, timeScheduled + 2, 1) != 0)
    {
        printf("Error, Incorrect format of date introduced\n");
        return EXIT_FAILURE;
    }

    strncpy(min, timeScheduled + 3, 2);

    if (strncmp(double_dot, timeScheduled + 5, 1) != 0)
    {
        printf("Error, Incorrect format of date introduced\n");
        return EXIT_FAILURE;   
    }
    
    strncpy(sec, timeScheduled + 6, 2);

    tm_timeScheduled->tm_hour = atoi(hour);
    tm_timeScheduled->tm_min = atoi(min);
    tm_timeScheduled->tm_sec = atoi(sec);

    if (tm_timeScheduled->tm_hour >= 24 || tm_timeScheduled->tm_hour < 0)
    {
        printf("Hour values range is between 0 and 23\n");
        return EXIT_FAILURE;
    }
    
    if (tm_timeScheduled->tm_min > 59 || tm_timeScheduled->tm_min < 0)
    {
        printf("Minute values range is between 0 and 59\n");
        return EXIT_FAILURE;
    }
    
    if (tm_timeScheduled->tm_sec > 59 || tm_timeScheduled->tm_sec < 0)
    {
        printf("Seconds values range is between 0 and 59\n");
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;

}

/**
 * @brief  Wait until the time is multiple of 15 minutes and 0 seconds to execute sweepings
 * @note   
 * @param tmScheduled: Time scheduled (input from scheduler.cfg reformated as date)
 * @retval Return 1 when time matches with the conditions of start XX:MM/15:00
 */
void startExecution(struct tm tmScheduled)
{
    time_t tStart;
    struct tm tmStart;
    char timeScheduledString[50];
    tStart = time(NULL);
    localtime_r(&tStart, &tmStart);
    
    strftime(timeScheduledString, sizeof(timeScheduledString), "%H:%M:%S", &tmScheduled);
    
    tStart = time(NULL);
    localtime_r(&tStart, &tmStart);
    
    printf("functions| startExecution() | Execution will start at %s\n", timeScheduledString);

    while(tmStart.tm_hour != tmScheduled.tm_hour ||
          tmStart.tm_min  != tmScheduled.tm_min  ||
          tmStart.tm_sec  != tmScheduled.tm_sec)
    {
        tStart = time(NULL);
        localtime_r(&tStart, &tmStart);
    }

    printf("functions| startExecution() | Execution Started\n");
}
int main(int argc, char **argv)
{
    int opt = 0, ii;
    char timeScheduled[50];
    struct tm tm_timeScheduled;

    while((opt = getopt(argc, argv, "t:h")) != EOF)
    {
        switch (opt)
        {
        case 't':
            strcpy(timeScheduled, optarg);
            if (formatStringToDate(timeScheduled, &tm_timeScheduled) == EXIT_FAILURE) { return EXIT_FAILURE; }
            
            break;

        case 'h':
            usage();
            break;
            
        default:
            usage();
            break;
        }
    }

    startExecution(tm_timeScheduled);
    return 0;
}
