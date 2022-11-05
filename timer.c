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

volatile int mark = 0;

void trigger(int signal)
{
    mark = 1;
}

int main()
{
    signal(SIGALRM, trigger);
    alarm(3);

}
