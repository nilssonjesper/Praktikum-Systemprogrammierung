/*! \file

Scheduling strategies used by the Interrupt Service RoutineA from Timer 2 (in scheduler.c)
to determine which process may continue its execution next.

The file contains five strategies:
-even
-random
-round-robin
-inactive-aging
-run-to-completion
*/

#include "os_scheduling_strategies.h"
#include "defines.h"
#include "util.h"

#include <stdlib.h>

// Global variables
SchedulingInformation schedulingInfo = {.priorities = {8, 4, 2, 1}};
	
uint8_t counter = 0;

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 *  \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
	switch(strategy){
		case OS_SS_ROUND_ROBIN:
			schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
			break;
		
		case OS_SS_INACTIVE_AGING:
			for(ProcessID i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
				schedulingInfo.age[i] = 0;
			}
			break;

		case OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE:
			// Init/Reset queues
			os_initSchedulingInformation();
			break;
		
		default:
			break;
	}
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID pid) {	
	schedulingInfo.age[pid] = 0;
	
	if(pid == 0){
		return;
	}
	
	Process* proc = os_getProcessSlot(pid);
	ProcessQueue* queue = MLFQ_getQueue(proc->priority >> 6);
	
	if(proc->state == OS_PS_UNUSED){
		pqueue_removePID(queue, pid);
		return;
	}
	
	pqueue_append(queue, pid);
	schedulingInfo.timeSlices[pid] = schedulingInfo.priorities[os_getProcessSlot(pid)->priority >> 6];

	return;
}



/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(const Process processes[], ProcessID current) {
	ProcessID pid = current % (MAX_NUMBER_OF_PROCESSES - 1);
	for(ProcessID i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[pid + 1].state == OS_PS_READY){
			return pid + 1;
		}
		
		pid = (pid + 1) % (MAX_NUMBER_OF_PROCESSES - 1);
	}
    return 0;
}

/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(const Process processes[], ProcessID current) {
	uint8_t ready_processes = 0;
	
	for(ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state == OS_PS_READY){
			ready_processes++;
		}
	}
	
	if(ready_processes == 0){
		return 0;
	}
	
	uint8_t lottery = rand() % ready_processes;
	lottery++;
	ready_processes = 0;
	
	for(ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state == OS_PS_READY){
			ready_processes++;
		}
		
		if(ready_processes == lottery){
			return i;
		}
	}
	
	return 0;
}

/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
ProcessID os_Scheduler_RoundRobin(const Process processes[], ProcessID current) {
	schedulingInfo.timeSlice--;

	if(schedulingInfo.timeSlice <= 0 || processes[current].state != OS_PS_READY){
		//Reseting timeSlice
		ProcessID pid = os_Scheduler_Even(processes, current);
		
		if(pid == 0){
			schedulingInfo.timeSlice = 1;
			return pid;
		}
		schedulingInfo.timeSlice = processes[pid].priority;
		return pid;
	}
    return current;
}

// MLFQ strategy
ProcessID os_Scheduler_MLFQ(const Process processes[], ProcessID current){
	counter++;
	// If idle process -> try to find new process
	if(current == 0){
		schedulingInfo.timeSlices[current] = 0;
	}
	
	if(counter == 31){
		delayMs(10);
	}
	
	Process* proc = os_getProcessSlot(current);
	Priority prio = proc->priority >> 6;
	ProcessQueue* currentQueue = MLFQ_getQueue(prio);
	
	if((schedulingInfo.timeSlices[current] <= 0 || proc->state == OS_PS_BLOCKED) && proc->state != OS_PS_UNUSED){
		pqueue_dropFirst(currentQueue);
		
		if(schedulingInfo.timeSlices[current] <= 0){
			proc->priority -= (prio == 0) ? 0:(1 << 6);
			os_resetProcessSchedulingInformation(current);
		} else {
			pqueue_append(currentQueue, current);
		}
	}
	
	// Find new process
	ProcessID pid = 0;
	ProcessQueue* queue = MLFQ_getQueue(NUMBER_OF_QUEUES - 1);
	for(int8_t i = (NUMBER_OF_QUEUES - 1); i >= 0; i--){
		queue = MLFQ_getQueue(i);
		ProcessID temp = pqueue_getFirst(queue);
		
		if(pqueue_hasNext(queue) && processes[temp].state == OS_PS_READY){
			pid = temp;
			break;
		}
	}
	
	schedulingInfo.timeSlices[pid]--;
	
	return pid;
}

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to 0.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(const Process processes[], ProcessID current) {
	ProcessID max = INVALID_PROCESS;
	for(ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state == OS_PS_UNUSED){
			continue;
		}
		
		if(processes[i].state == OS_PS_READY){
			max = i;
		}
		
		schedulingInfo.age[i] += processes[i].priority;
	}
	
	if(max == INVALID_PROCESS) return 0;
	
	for(ProcessID i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state != OS_PS_READY) continue;
		
		if(schedulingInfo.age[i] > schedulingInfo.age[max]){
			max = i;
		}
		
		else if(schedulingInfo.age[i] == schedulingInfo.age[max]){
			max = (processes[i].priority > processes[max].priority) ? i : max;
		}
	}
	
	schedulingInfo.age[max] = 0;
	return max;
}

/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(const Process processes[], ProcessID current) {
	if(processes[current].state == OS_PS_READY && current != 0){
		return current;
	}
	
	return os_Scheduler_Even(processes, current);
}

SchedulingInformation os_getSchedulingInfo(void){
	return schedulingInfo;
}

// MLFQ functions
ProcessQueue* MLFQ_getQueue(uint8_t index){
	switch((index)){
		case 0:
			return &schedulingInfo.firstQueue;
		case 1:
			return &schedulingInfo.secondQueue;
		case 2:
			return &schedulingInfo.thirdQueue;
		case 3:
			return &schedulingInfo.fourthQueue;
		default:
			return NULL;
	}
}

void os_initSchedulingInformation(void){
	// Reset/Init all queues
	for(uint8_t i = 0; i < NUMBER_OF_QUEUES; i++){
		pqueue_init(MLFQ_getQueue(i));
	}
	
	// Reset running process first
	for(ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++){
		if(os_getProcessSlot(pid)->state == OS_PS_RUNNING){
			os_resetProcessSchedulingInformation(pid);
			break;
		}
	}
	
	// Reset all current processes
	for(ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++){
		if(os_getProcessSlot(pid)->state != OS_PS_UNUSED && os_getProcessSlot(pid)->state != OS_PS_RUNNING){
			os_resetProcessSchedulingInformation(pid);
		}
	}
}


// Process queue functions
void pqueue_init(ProcessQueue* queue){
	queue->size = MAX_NUMBER_OF_PROCESSES;
	queue->head = 0;
	queue->tail = 0;
}

void pqueue_reset(ProcessQueue* queue){
	queue->head = 0;
	queue->tail = 0;
}

uint8_t pqueue_hasNext(ProcessQueue const* queue){
	return queue->head != queue->tail;
};

ProcessID pqueue_getFirst(ProcessQueue const* queue){
	if(!pqueue_hasNext(queue)){
		return INVALID_PROCESS;
	}
	
	return queue->data[queue->tail];
}

ProcessID pqueue_dropFirst(ProcessQueue* queue){
	if(!pqueue_hasNext(queue)){
		return INVALID_PROCESS;
	}
	
	ProcessID pid = queue->data[queue->tail];
	queue->tail = (queue->tail + 1) % queue->size;
	return pid;
}

uint8_t pqueue_append(ProcessQueue* queue, ProcessID pid){
	if((queue->head + 1) % queue->size == queue->tail){
		return 0;
	}

	queue->data[queue->head] = pid;
	queue->head = (queue->head + 1) % queue->size;
	return 1;
}

uint8_t pqueue_removePID(ProcessQueue* queue, ProcessID pid){
	uint8_t current = queue->tail;
	while(current != queue->head){
		if(queue->data[current] == pid){
			// Start shifting elements
			while(current != queue->tail){
				uint8_t new = (current == 0) ? (queue->size - 1):(current - 1);
				queue->data[current] = queue->data[new];
				current = new;
			}

			queue->tail = (queue->tail + 1) % queue->size;
			return 1;
		}

		current = (current + 1) % queue->size;
	}
	return 0;
}