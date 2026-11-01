#include "osKernel.h"

#define NUM_OF_THREADS			3
#define STACKSIZE				100

#define BUS_FREQ				16000000

#define CTRL_ENABLE		(1U<<0)
#define CTRL_TICKINT	(1U<<1)
#define CTRL_CLCKSRC	(1U<<2)
#define CTRL_COUNTFLAG 	(1U<<16)
#define SYSTICK_RST		0


void osSchedulerLaunch(void);

uint32_t 	MILLIS_PRESCALER;

struct tcb{
	int32_t *stackPt;
	struct tcb *nextPt;
};

typedef struct tcb tcbType;

tcbType tcbs[NUM_OF_THREADS];
tcbType *currentPt;

/* Each thread will have stacksize of 100 i.e. 400bytes */
int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

void osKernelStackInit(int i)
{
	tcbs[i].stackPt = &TCB_STACK[i][STACKSIZE - 16];

	/* Set bit 21 (T-bit) in PSR to 1, to operate in thumb mode*/
	TCB_STACK[i][STACKSIZE-1] = (1U<<24);

	/* Dummy Stack Content */
	TCB_STACK[i][STACKSIZE-3]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-4]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-5]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-6]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-7]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-8]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-9]  = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-10] = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-11] = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-12] = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-13] = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-14] = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-15] = 0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-16] = 0xAAAAAAAA;

}

uint8_t osKernelAddThreads(void (*task0)(void),void (*task1)(void),void (*task2)(void))
{
	//* Disable Global Interrupts*//
	__disable_irq();
	tcbs[0].nextPt = &tcbs[1];
	tcbs[1].nextPt = &tcbs[2];
	tcbs[2].nextPt = &tcbs[0];

	/* Init Stack for thread 0*/
	osKernelStackInit(0);
	/* Init PC*/
	TCB_STACK[0][STACKSIZE-2] = (int32_t)(task0);

	/* Init Stack for thread 1*/
	osKernelStackInit(1);
	/* Init PC*/
	TCB_STACK[1][STACKSIZE-2] = (int32_t)(task1);

	/* Init Stack for thread 2*/
	osKernelStackInit(2);
	/* Init PC*/
	TCB_STACK[2][STACKSIZE-2] = (int32_t)(task2);

	/* Start from Thread 0*/
	currentPt = &tcbs[0];

	/*Enable Global Interrupts*/
	__enable_irq();

	return 1;

}

void osKernelInit(void)
{
	MILLIS_PRESCALER = (BUS_FREQ/1000);
}

void osKernelLaunch(uint32_t quanta)
{
	/*Reset Systick*/
	SysTick->CTRL = SYSTICK_RST;

	/*Clear Systick current value register*/
	SysTick->VAL = 0;

	/*Load quanta*/
	SysTick->LOAD = (quanta * MILLIS_PRESCALER) - 1;

	/*Set Systick to low priority*/
	NVIC_SetPriority(SysTick_IRQn,15);

	/*Enable Systick, select internal clock*/
	SysTick->CTRL = CTRL_CLCKSRC | CTRL_ENABLE;

	/*Enable Systick interrupt*/
	SysTick->CTRL |= CTRL_TICKINT;

	/*Launch Scheduler*/
	osSchedulerLaunch();
}

/*When exception occurs these registers are automatically
 * saved onto the stack : r0,r1,r2,r3,r12,lr,pc,psr*/

__attribute__((naked)) void SysTick_Handler(void)
{
	/*SUSPEND CURRENT THREAD*/


	/*Disable Global Interrupts*/
	__asm("CPSID     I");

	/*Save r4,r5,r6,r7,r8,r9,r10,r11*/
	__asm("PUSH {R4-R11}");

	/*Load Address of currentPt into r0*/
	__asm("LDR R0, =currentPt");

	/*Load r1 from address equals r0, i.e. r1 =currentPt*/
	__asm("LDR R1,[R0]");

	/*Store Cortex-M SP at address equals r1, i.e. Save SP into tcb*/
	__asm("STR SP,[R1]");

	/*Choose Next Thread*/
	/*Load r1 from a location 4bytes address r1, i.e. r1 = curentPt->next*/
	__asm("LDR R1,[R1,#4]");

	/*Store r1 at address equals r0, i.e. currentPt = r1*/
	__asm("STR R1,[R0]");

	/*Load Cortex-M SP from address equals r1, i.e. SP = currentPt->stackPt*/
	__asm("LDR SP,[R1]");

	/*Restore r4,r5,r6,r7,r8,r9,r10,r11*/
	__asm("POP {R4-R11}");

	/*Enable Global Interrupts*/
	__asm("CPSIE     I");

	/*Return from exception and restore r0,r1,r2,r3,r12,lr,pc,psr*/
	__asm("BX   LR");

}

void osSchedulerLaunch(void)
{
	/*Load address of currentPt into R0*/
	__asm("LDR R0,=currentPt");

	/*Load r2 from address equals r0,i.e. r2 =currentPt*/
	__asm("LDR R2,[r0]");

	/*Load Cortex-M SP from address equals R2, i.e. SP = currentPt-> stackPt*/
	__asm("LDR SP,[R2]");
	/*Restore r4,r5,r6,r7,r8,r9,r10,r11*/
	__asm("POP {R4-R11}");

	/*Restore r12*/
	__asm("POP {R12}");

	/*Restore r0,r1,r2,r3*/
	__asm("POP {R0-R3}");

	/*Skip LR and PSR*/
	__asm("ADD SP,SP,#4");

	/*Create a new start location by popping LR*/
	__asm("POP {LR}");

	/*Skip PSR by adding 4 to SP*/
	__asm("ADD SP,SP,#4");

	/*Enable Global Interrupts*/
	__asm("CPSIE     I");

	/*Return from exception*/
	__asm("BX   LR");

}
