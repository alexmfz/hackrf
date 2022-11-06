/**
 * @file timer.c
 * @author Alejandro
 * @brief 
 * @version 0.1
 * @date 2022-11-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define ITERATIONS  10
#define INTERVAL 25e4 //us => 25e4 us = 250 ms
#define MICRO   1e6

volatile int mark = 0;

struct itimerval timer;    
struct timeval timeval;
float durationIteration = 0;

void timerHandler(int sig)
{
    mark = 1;
} 

void setTimerParams()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = INTERVAL;
    timer.it_interval = timer.it_value;
}

float hackRFTrigger(long previousTime, long postTime)
{
    gettimeofday(&timeval,NULL);
    previousTime = timeval.tv_sec*1e6 + timeval.tv_usec;
    printf("hackRFTrigger | Initial time:%ld\n", previousTime);

    if(signal(SIGALRM, timerHandler) == SIG_ERR)
    {
        perror("hackRFTrigger | Unable to catch alarm signal");
        return 0;
    }
   
    if(setitimer(ITIMER_REAL, &timer, 0) == -1)
    {
        perror("hackRFTrigger | Error calling timer");
        return 0;
    }
    
    while(!mark){ }
    gettimeofday(&timeval,NULL);
    postTime = timeval.tv_sec*1e6 + timeval.tv_usec;

    durationIteration = (float)(postTime-previousTime)/MICRO;
    mark = 0;  

    printf("hackRFTrigger | Final time:%ld\n", postTime);
    printf("hackRFTrigger | Duration: %.2f s\n", (float)(postTime-previousTime)/MICRO);
    
    return durationIteration;
}

/*int main()
{
    long previousTime = 0 , postTime = 0;
    float totalDuration = 0;
    
    setTimerParams();

    for(int i = 0; i < ITERATIONS; i++)
    {
        printf("Iteration %d:\n", i+1);
        totalDuration += hackRFTrigger(previousTime, postTime);
    }

    printf("\n\nIterations: %d\nTotal duration: %f s\n", ITERATIONS, totalDuration);
    return 0;    
}*/