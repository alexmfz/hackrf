#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define INTERVAL 25e4 //us => 25e4 us = 250 ms

struct itimerval timer;    
struct timeval preTriggering;
struct timeval postTriggering;

int timerFlag = 0;

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

int main(int argc, char** argv) 
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
    
    return 0;
}