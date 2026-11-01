# STM32_RR_Scheduler_From_Scratch

A round-robin scheduler for an STM32, written from scratch in C and ARM assembly. No FreeRTOS and no CMSIS-RTOS: just thread control blocks, hand-built stacks, and a SysTick-driven context switch.

## Features
- Three cooperating threads, each with its own 400-byte stack
- Thread control blocks (TCBs) linked in a circular list
- SysTick interrupt triggers the context switch at a configurable time slice
- Context switch written in assembly: save R4-R11, swap stack pointers, restore the next thread
- First run of each thread set up by hand with a fake exception frame
- Per-task profiler counters to see how CPU time is shared
- Built with STM32CubeIDE

## How it works
1. **Thread control block:** each thread has a stack pointer and a pointer to the next thread. The three TCBs form a ring, so the scheduler always knows who runs next.
2. **Stack initialization:** each stack is prepared to look like the thread was already interrupted. The top of the stack holds the Thumb bit in xPSR, the thread's entry function as the PC, and dummy values for the other registers.
3. **Launch:** `osKernelLaunch(quanta)` configures SysTick for the time slice, then `osSchedulerLaunch()` loads the first thread's stack and returns into it as if from an exception.
4. **Context switch:** on every SysTick interrupt, the hardware pushes R0-R3, R12, LR, PC and xPSR. The handler pushes R4-R11, saves the stack pointer into the current TCB, loads the next TCB's stack pointer, pops R4-R11, and returns into the next thread.

```
SysTick fires -> save R4-R11 -> save SP in current TCB
              -> currentPt = currentPt->next
              -> load SP from new TCB -> restore R4-R11 -> return
```

## Stack layout (per thread)
| Stack position | Content |
|----------------|---------|
| Top | xPSR (Thumb bit set) |
| Top - 1 | PC (thread entry function) |
| Top - 2 | LR |
| Top - 3 to Top - 6 | R12, R3, R2, R1, R0 area |
| Top - 7 to Top - 14 | R4-R11 (saved by the handler) |

## Usage
```c
#define QUANTA 1000   // time slice in ms

int main()
{
    uart_tx_init();
    osKernelInit();
    osKernelAddThreads(&task0, &task1, &task2);
    osKernelLaunch(QUANTA);
}
```
Each task is a normal function with an infinite loop. The profiler counters (`Task0_Profiler`, `Task1_Profiler`, `Task2_Profiler`) increase while their task is running, so you can watch the time being shared.

## Getting started
1. Open the project in STM32CubeIDE.
2. Build and flash it with ST-Link.
3. Open a serial terminal and watch the output from the running task.
4. Add the profiler counters to the debugger's live expressions to see each task's share of CPU time.

## Roadmap
- [ ] Periodic scheduler
- [ ] Semaphores and inter-task communication (IPC)
- [ ] Priority scheduling
