/*! \file
 *  \brief Scheduling module for the OS.
 *
 * Contains everything needed to realise the scheduling between multiple processes.
 * Also contains functions to start the execution of programs.
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#include "os_scheduler.h"

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "util.h"

#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdbool.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
Process os_processes[MAX_NUMBER_OF_PROCESSES];
uint8_t numberOfAutostartProcs = 0;

//! Array of states for every possible process
ProcessState os_process_states[MAX_NUMBER_OF_PROCESSES];

//! Index of process that is currently executed (default: idle)
ProcessID currentProc = 0;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
SchedulingStrategy currentStrat = OS_SS_EVEN;

//! Count of currently nested critical sections
uint8_t criticalSectionCount = 0;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect)
__attribute__((naked));

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
	saveContext();
	os_processes[os_getCurrentProc()].sp.as_int = SP;
	os_processes[os_getCurrentProc()].state = OS_PS_READY;
	SP = BOTTOM_OF_ISR_STACK;
	os_processes[os_getCurrentProc()].checksum = os_getStackChecksum(os_getCurrentProc());

	// Check if Enter + ESC is pressed
	if(os_getInput() == 0b00001001 && !os_taskManOpen()){
		os_waitForNoInput();
		os_taskManMain();
	}
	
	ProcessID pid = os_getNextProcessID();
	if(os_processes[pid].checksum != os_getStackChecksum(pid)){
		os_error("Stack inconsistency detected!");
	}
	os_processes[pid].state = OS_PS_RUNNING;
	SP = os_processes[pid].sp.as_int;
	currentProc = pid;
	restoreContext();
}

ProcessID os_getNextProcessID(void){
	switch (os_getSchedulingStrategy())
	{
		case OS_SS_RANDOM:
		return os_Scheduler_Random(os_processes, os_getCurrentProc());

		case OS_SS_EVEN:
		return os_Scheduler_Even(os_processes, os_getCurrentProc());
		
		case OS_SS_ROUND_ROBIN:
		return os_Scheduler_RoundRobin(os_processes, os_getCurrentProc());
		
		case OS_SS_INACTIVE_AGING:
		return os_Scheduler_InactiveAging(os_processes, os_getCurrentProc());
		
		case OS_SS_RUN_TO_COMPLETION:
		return os_Scheduler_RunToCompletion(os_processes, os_getCurrentProc());
		
		default:
		os_error("Invalid scheduling strategy selected!");
		return 0;
	}
}


/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
void idle(void){
	while(1){
		lcd_clear();
		lcd_writeProgString(PSTR("..."));
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

/*!
 *  This function is used to register the given program for execution.
 *  A stack will be provided if the process limit has not yet been reached.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param program  The function of the program to start.
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favourable
 *                   - 255 means most favourable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process or INVALID_PROCESS as specified in
 *          defines.h on failure
 */
ProcessID os_exec(Program *program, Priority priority) {
	os_enterCriticalSection();
	// Find free place in os_processes
	ProcessID pid = 0;
	
	while(pid < MAX_NUMBER_OF_PROCESSES && os_processes[pid].state != OS_PS_UNUSED){
		pid++;
	}
	
	// Check if free place has been found
	if(pid >= MAX_NUMBER_OF_PROCESSES){
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}
	
	// Check if program-pointer is NULL
	if(program == NULL){
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}
	
	// Get free process-slot
	Process *process = &os_processes[pid];
	
	// Set parameters accordingly
	process->state = OS_PS_READY;
	process->program = program;
	process->priority = priority;
	
	// Preparing stack
	process->sp.as_int = PROCESS_STACK_BOTTOM(pid);
	*(process->sp.as_ptr--) = (uint8_t) ((uint16_t)program); 
	*(process->sp.as_ptr--) = ((uint16_t) program) >> 8;
	
	
	// 33 zero-bytes for the 33 registers
	for(uint8_t i = 0; i < 33; i++){
		*(process->sp.as_ptr--) = 0;
	}
	
	process->checksum = os_getStackChecksum(pid);
	
	// Set age for process (inactiveAging)
	os_resetProcessSchedulingInformation(pid);
	
	os_leaveCriticalSection();
	return pid;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
	// Set running process to idle
	currentProc = 0;
	Process *proc = &os_processes[0];
	proc->state = OS_PS_RUNNING;
	SP = proc->sp.as_int;
	restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void){
	// Init states to UNUSED
	for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		os_processes[i].state = OS_PS_UNUSED;
		os_processes[i].priority = 0;
		os_processes[i].program = NULL;
		os_processes[i].checksum = 0;
	}
	
	// Starting idle process
	os_exec(idle, DEFAULT_PRIORITY);
	
	// Inserting autostart-processes into os_processes
	struct program_linked_list_node *current = autostart_head;
	while(current != NULL){
		os_exec(current->program, DEFAULT_PRIORITY);
		current = current->next;
		numberOfAutostartProcs++;
	}
	
	return;
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process *os_getProcessSlot(ProcessID pid) {
    return &os_processes[pid];
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
    return currentProc;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy){
	currentStrat = strategy;
	os_resetSchedulingInformation(currentStrat);
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    return currentStrat;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behaviour when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
	// Disable global-interrupt-enable
	uint8_t gib = (SREG >> 7);
	SREG &= 0b01111111;
	
	if(criticalSectionCount >= 255){
		SREG |= gib << 7;
		os_error("Too many critical sections!");
		return;
	}
	
	// Increase critical-section counter
	criticalSectionCount++;
	
	// Disable scheduler-interrupt (timer interrupt)
	TIMSK2 &= ~(1 << OCIE2A);
	
	//Enable global-interrupt-enable
	SREG |= gib << 7;
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void){
	// Disable global-interrupt-enable
	uint8_t gib = (SREG >> 7);
	SREG &= 0b01111111;
	
	if(criticalSectionCount == 0){
		SREG |= gib << 7;
		os_error("No critical section to leave!");
		return;
	}
	
	criticalSectionCount--;

	// Disable scheduler-interrupt (timer interrupt)
	if(criticalSectionCount == 0){
		TIMSK2 |= 1 << OCIE2A;
	}
	
	// END OF ATOMIC
	SREG |= gib << 7;
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
	Process proc = os_processes[pid];
	StackPointer sp = proc.sp;
	uint16_t bottom = PROCESS_STACK_BOTTOM(pid);
	StackChecksum sum = *(++sp.as_ptr);
	
	while(++sp.as_int <= bottom){
		sum ^= *(sp.as_ptr);
	}
    return sum;
}
