#include "sched.h"

#define MAX_TASKS 12
#define TASK_STACK_WORDS 256        //1kB = 256 * (4 bytes)


typedef struct Task_t {
    uint32_t *sp;

    uint32_t *stack_bottom;
    uint32_t *stack_top;

    uint32_t stack_size;
    
    uint8_t state;      //to be implemented later
    uint8_t priority;   //to be implemented later

}Task_t;

static uint8_t total_tasks = 0;

static Task_t TCB[MAX_TASKS] = {0};

static uint32_t stacks[MAX_TASKS][TASK_STACK_WORDS];

static uint8_t current_task_num = 0;
static Task_t *current_task;

static void TaskExit(void)
{
    while(1);
}


void OS_Init()
{
    total_tasks = 0;

    current_task_num = 0;

    current_task = 0;

    for(uint32_t i=0; i<MAX_TASKS; i++)
    {
        TCB[i].sp = 0;
        TCB[i].stack_bottom = 0;
        TCB[i].stack_top = 0;
        TCB[i].stack_size = 0;
        TCB[i].state = 0;
        TCB[i].priority = 0;
    }
}

uint32_t OS_create_task(void(*task_func)(void))
{
    Task_t *task = &TCB[total_tasks];
    task->stack_bottom = &stacks[total_tasks][0];           //not used at the moment but only initialised just in case
    task->stack_top = &stacks[total_tasks][TASK_STACK_WORDS];

    task->stack_size = TASK_STACK_WORDS;
    uint32_t *sp = task->stack_top;

    //Hardware stack frame(Assumed to be pushed by hardware when an ISR is called)
    *(--sp) = 0x01000000;               //xPSR
    *(--sp) = (uint32_t) task_func;     //PC (the actual task function)
    *(--sp) = (uint32_t) TaskExit;      //LR (this is a failsafe in case a task exits unintentionally.)
    *(--sp) = 0;                        //R12
    *(--sp) = 0;                        //R3
    *(--sp) = 0;                        //R2
    *(--sp) = 0;                        //R1
    *(--sp) = 0;                        //R0

    //Software stack frame(what will be actually pushed and restored by the PendSV_Handler for context switch)
    *(--sp) = 0;            //R11
    *(--sp) = 0;            //R10
    *(--sp) = 0;            //R9
    *(--sp) = 0;            //R8
    *(--sp) = 0;            //R7
    *(--sp) = 0;            //R6
    *(--sp) = 0;            //R5
    *(--sp) = 0;            //R4

    task->sp = sp;

    total_tasks++;

    return total_tasks - 1;
}

void OS_start_tasks(void)       //taken from FreeRTOS implementation
{
    current_task_num = 0;
    current_task = &TCB[0];

    __asm volatile
    (
        /* Restore the MSP to the reset value from the vector table. */
        "ldr r0, =0xE000ED08     \n"   /* SCB->VTOR */
        "ldr r0, [r0]            \n"   /* Vector table address */
        "ldr r0, [r0]            \n"   /* Initial MSP */
        "msr msp, r0             \n"

        /* Ensure Thread mode uses MSP before entering SVC. */
        "movs r0, #0             \n"
        "msr CONTROL, r0         \n"
        "isb                     \n"

        /* Enable interrupts. */
        "cpsie i                 \n"
        "cpsie f                 \n"
        "dsb                     \n"
        "isb                     \n"

        /* Enter the kernel. */
        "svc 0                   \n"

        "nop                     \n"

        :
        :
        : "r0"
    );
}

void OS_yield() 
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

//Only used in this file.
void Scheduler_SelectNextTask()
{
    current_task_num++;
    if(current_task_num == total_tasks) current_task_num = 0;
    current_task = &TCB[current_task_num];
}

//Context switch mechanism
__attribute__((naked)) void PendSV_Handler()
{
    __asm volatile
    (
        "mrs r0, psp                    \n"     //get the process stack pointer
        "stmdb r0!, {r4-r11}            \n"     //push registers {r4-r11} onto the stack
        
        "ldr r1, =current_task          \n"     //loads the address of variable current_task
        "ldr r1,[r1]                    \n"     //extract the task's stack pointer (i.e. the task's struct's 0th location)
        "str r0,[r1]                    \n"     //save the stack pointer value 

        "push {lr}                      \n"     //save the lr (value 0xFFFFFFFD) to the main stack
        "bl Scheduler_SelectNextTask    \n"     //normally calling a C function
        "pop {lr}                       \n"     //restoring the previous saved lr

        "ldr r1, =current_task          \n"     //new task selected by scheduler
        "ldr r1,[r1]                    \n"     //extract the task's stack pointer
        "ldr r0,[r1]                    \n"     //get the task's stack pointer

        "ldmia r0!, {r4-r11}            \n"     //restore registers {r4-r11} onto the stack
        "msr psp, r0                    \n"     //restore the new task's psp
        "isb                            \n"
        "bx lr                          \n"     //get out of the ISR
    );
}

__attribute__((naked))
void SVC_Handler(void)
{
    __asm volatile
    (
        "ldr r3, =current_task      \n"
        "ldr r1, [r3]               \n"
        "ldr r0, [r1]               \n"

        "ldmia r0!, {r4-r11}        \n"

        "msr psp, r0                \n"
        "isb                        \n"

        "ldr lr, =0xFFFFFFFD        \n"
        "bx lr                      \n"
    );
}

//use systick_handler() to implement preemptive scheduling