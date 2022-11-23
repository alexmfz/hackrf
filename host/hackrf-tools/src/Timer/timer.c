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
#include "/home/manolo/hackrf/host/hackrf-tools/src/Functions/functions.h"

#define INTERVAL 25e4 //us => 25e4 us = 250 ms

struct itimerval timer;    
struct timeval preTriggering;
struct timeval postTriggering;


/**
 * @brief  Configurates timer parameters
 * @note   
 * @retval None
 */
void setTimerParams()
{
    printf ("timer | setTimerParams() | Configuration timer parameters\n");
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = INTERVAL;
    timer.it_interval = timer.it_value;

    printf("timer | setTimerParams() | Value interval: %ld us\n", timer.it_interval.tv_usec);
    printf("timer | setTimerParams() | Execution Success\n");
}