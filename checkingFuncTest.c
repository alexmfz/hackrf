#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define INTERVAL 25e4 //us => 25e4 us = 250 ms

struct itimerval timer;    
struct timeval preTriggering;
struct timeval postTriggering;

int timerFlag = 0;
char timeDatas[3600][50] = {""}; // Time Datas of the sweeping | 3600 dates
float *samples;

void timerHandler(int sig)
{
    timerFlag = 1;
} 

void setTimerParams()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = INTERVAL;
    timer.it_interval = timer.it_value;

    printf("timer | setTimerParams | Value interval: %ld us\n", timer.it_interval.tv_usec);
    printf("timer | setTimerParams | Success\n");
}


void execTimer()
{
 if(signal(SIGALRM, timerHandler) == SIG_ERR)
    {
        fprintf(stderr, "timer | hackRFTrigger | Unable to catch alarm signal");
        return ;
    }
   
    if(setitimer(ITIMER_REAL, &timer, 0) == -1)
    {
        fprintf(stderr, "timer | hackRFTrigger | Error calling timer");
        return ;
    }
    
}

static float TimevalDiff(const struct timeval *a, const struct timeval *b) {
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

void timerNewConceptTest()
{
    int initTimer = 0, iteration = 0;
    setTimerParams();

    gettimeofday(&preTriggering, NULL);
    execTimer();
    while(1)
    {
        if (timerFlag == 1)
        {
            fprintf(stderr, "Iteration %d: %f s\n", iteration, TimevalDiff(&postTriggering, &preTriggering));
            gettimeofday(&postTriggering, NULL);
            timerFlag = 0;
            iteration++;
            execTimer();
        }
    }

}

int saveTimesTest(int i, int triggeringTimes, char* sweepingTime)
{
    if ( i == 0)
    {
        printf("generationFits | saveTimes() | Start saving index data to generate fits file\n");
    }
    
     // Saving step
    strcpy(timeDatas[i], sweepingTime);

    // Checks
    if (strcmp(timeDatas[i], "") == 0)
    {
        fprintf(stderr, "generationFits | saveTimes() | Was not possible to save timing data\n");
        return -1;
    }

    if ( i == triggeringTimes-1)
    {
        printf("generationFits | saveTimes() | Execution Sucess\n");
    }

   
    return 0;
}

void saveTimesLoopTest(char *sweepingTime)
{
    int i = 0; 
    int triggeringTimes = 10;
    for (i = 0; i < triggeringTimes; i++)
        if (saveTimesTest( i, triggeringTimes, sweepingTime) == -1)
        {
            fprintf(stderr, "Exiting\n");
            return;
        }

    printf("Printing results:\n");
    for (i = 0; i < triggeringTimes; i++)
        printf("Result %i: %s\n", i, timeDatas[i]);
}

int saveSamplesTest(int i, float powerSample, int nElements)
{
    if (i == 0)
    {
        printf("generationFits | saveSamples() | Start saving power sample data to generate fits file\n");
        samples = (float*)calloc(nElements,sizeof(float));
        samples[i] = powerSample;
    }

    else
    {
        samples[i] = powerSample;
    }
    
    if (samples == NULL)
    {
        fprintf(stderr, "generationFits | saveSamples() | Was not possible to save power samples\n");
        return -1;
    }

    if (i == nElements-1)
    {
        printf("generationFits | saveSamples() | Execution Sucess\n");
    }

    return 0;
}

void saveSamplesLoopTest()
{
    int i = 0, j = 0; 
    int triggeringTimes = 10;

    for (i = 0; i < triggeringTimes; i++)
    {
        for (j = i*(50/triggeringTimes); j < (50/triggeringTimes)*(i+1); j++)
        {
            saveSamplesTest(j, j, 50);
        }
    }
    printf("\n");

    for (i = 0; i < triggeringTimes; i++)
    {
        for (j = i*(50/triggeringTimes); j < (50/triggeringTimes)*(i+1); j++)
        {
            printf("Value[%d]: %f\n", j, samples[j]);
        }
    }
     

}

int main(int argc, char** argv) 
{
    /*fprintf(stderr, "timerNewConceptTest()\n");
    timerNewConceptTest(); // Is in a while(1), discomment to test*/
    
    fprintf(stderr, "saveTimesLoopTest()\n");
    saveTimesLoopTest("Sweeping Time");
    printf("\n");
    printf("Press 'Enter' to continue....");
    while(getchar()!='\n');

    saveTimesLoopTest("");
    printf("\n");
    printf("Press 'Enter' to continue....");
    while(getchar()!='\n');

    gettimeofday(&preTriggering, NULL);
    saveSamplesLoopTest();
    gettimeofday(&postTriggering, NULL);
    printf("Time of saving samples: %f s\n", TimevalDiff(&postTriggering, &preTriggering));
    printf("\n");
    printf("Press 'Enter' to continue....");
    while(getchar()!='\n');
    
    return 0;
}