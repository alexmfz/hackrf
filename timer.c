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
#define INTERVAL 250000 //us
#define MICRO   1e6
volatile int mark = 0;

void timerHandler(int sig)
{
    mark = 1;
} 

void iteration(long previousTime, long postTime)
{
    struct itimerval timer;    
    struct timeval timeval;

    gettimeofday(&timeval,NULL);
    previousTime = timeval.tv_sec*1e6 + timeval.tv_usec;
    printf("Time:%ld\n", previousTime);

    if(signal(SIGALRM, timerHandler) == SIG_ERR)
    {
        perror("Unable to catch alarm signal");
        return ;
    }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = INTERVAL;
    timer.it_interval = timer.it_value;
    if(setitimer(ITIMER_REAL, &timer, 0) == -1)
    {
        perror("Error calling timer");
        return ;
    }
    
    while(!mark){ }
    gettimeofday(&timeval,NULL);
    postTime = timeval.tv_sec*1e6 + timeval.tv_usec;
    printf("Time:%ld\n", postTime);
    printf("Duration: %.2f s\n", (float)(postTime-previousTime)/1e6);
    
    mark = 0;  
}

int main()
{
    long previousTime = 0 , postTime = 0;
 
    for(int i = 0; i < ITERATIONS; i++)
    {
        printf("Iteration %d:\n", i+1);
        iteration(previousTime, postTime);
    }
    return 0;    
}