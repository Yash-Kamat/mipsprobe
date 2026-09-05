#ifndef SCHED_H
#define SCHED_H

#include "stm32l4xx.h"

typedef void(*task_t) (void);


void OS_Init();

//create a task in the tcb array and return it's task number;
uint32_t OS_create_task(void(*task_func)(void));

//supposed to be run only once in the function that creates all tasks.
void OS_start_tasks();

//for tasks to yield their time quantum
void OS_yield();

#endif