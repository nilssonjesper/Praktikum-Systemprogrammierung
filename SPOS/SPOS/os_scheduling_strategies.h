/*! \file
 *  \brief Scheduling library for the OS.
 *
 *  Contains the scheduling strategies.
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#ifndef _OS_SCHEDULING_STRATEGIES_H
#define _OS_SCHEDULING_STRATEGIES_H

#include "defines.h"
#include "os_scheduler.h"

typedef struct {
    ProcessID data[MAX_NUMBER_OF_PROCESSES];
    uint8_t size;
    uint8_t head;
    uint8_t tail;
} ProcessQueue;

//! Structure used to store specific scheduling informations such as a time slice
// This is a presence task
typedef struct {
	uint8_t timeSlice;
	Age age[MAX_NUMBER_OF_PROCESSES];
	uint8_t timeSlices[MAX_NUMBER_OF_PROCESSES];
	uint8_t priorities[NUMBER_OF_QUEUES];
	ProcessQueue firstQueue;
	ProcessQueue secondQueue;
	ProcessQueue thirdQueue;
	ProcessQueue fourthQueue;
} SchedulingInformation;



//! Getter for the SchedulingInformation
SchedulingInformation os_getSchedulingInfo(void);

//! Used to reset the SchedulingInfo for one process
void os_resetProcessSchedulingInformation(ProcessID id);

//! Used to init the SchedulingInfo for one process
void os_initProcessSchedulingInformation(ProcessID pid);

//! Used to reset the SchedulingInfo for a strategy
void os_resetSchedulingInformation(SchedulingStrategy strategy);

//! Even strategy
ProcessID os_Scheduler_Even(const Process processes[], ProcessID current);

//! Random strategy
ProcessID os_Scheduler_Random(const Process processes[], ProcessID current);

//! RoundRobin strategy
ProcessID os_Scheduler_RoundRobin(const Process processes[], ProcessID current);

//! InactiveAging strategy
ProcessID os_Scheduler_InactiveAging(const Process processes[], ProcessID current);

//! RunToCompletion strategy
ProcessID os_Scheduler_RunToCompletion(const Process processes[], ProcessID current);


// ----------------------------------------------------------------------------
// Multi-level feedback queue strategy
// ----------------------------------------------------------------------------

ProcessQueue* MLFQ_getQueue(uint8_t index);
void os_initSchedulingInformation(void);
ProcessID os_Scheduler_MLFQ(const Process processes[], ProcessID current);

void pqueue_init(ProcessQueue* queue);
void pqueue_reset(ProcessQueue* queue);
uint8_t pqueue_hasNext(ProcessQueue const* queue);
ProcessID pqueue_getFirst(ProcessQueue const* queue);
ProcessID pqueue_dropFirst(ProcessQueue* queue);
uint8_t pqueue_append(ProcessQueue* queue, ProcessID pid);
uint8_t pqueue_removePID(ProcessQueue* queue, ProcessID pid);
#endif
