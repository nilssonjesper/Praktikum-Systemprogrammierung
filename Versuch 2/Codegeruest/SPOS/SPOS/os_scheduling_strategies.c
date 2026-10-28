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
#include "lcd.h"
#include "defines.h"

#include <stdlib.h>

// Global variables
SchedulingInformation schedulingInfo;

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
	
	if(schedulingInfo.timeSlice <= 0){
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
