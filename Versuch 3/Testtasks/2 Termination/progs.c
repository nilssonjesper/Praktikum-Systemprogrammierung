//-------------------------------------------------
//          TestTask: Termination
//-------------------------------------------------

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_memory.h"
#include "os_scheduler.h"
#include "os_taskman.h"
#include "util.h"
#include "os_scheduling_strategies.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#if VERSUCH < 3
#error "Please fix the VERSUCH-define"
#endif

//---- Adjust here what to test -------------------
#define PHASE_1 1
#define PHASE_2 1
//! Defines how often each strategy is tested
#define RUNS 2
//! Defines after how many spawns the strategy is changed
#define MAX_SPAWNS 200
//-------------------------------------------------

#ifndef WRITE
#define WRITE(str) lcd_writeProgString(PSTR(str))
#endif
#define TEST_PASSED                    \
    do {                               \
        ATOMIC {                       \
            lcd_clear();               \
            WRITE("  TEST PASSED   "); \
        }                              \
    } while (0)
#define TEST_FAILED(reason)  \
    do {                     \
        ATOMIC {             \
            lcd_clear();     \
            WRITE("FAIL  "); \
            WRITE(reason);   \
        }                    \
    } while (0)
#ifndef CONFIRM_REQUIRED
#define CONFIRM_REQUIRED 1
#endif

// Flag indicating an error
static bool errflag = false;

static int stderrWrapper(const char c, FILE *stream) {
    errflag = true;
    putchar(c);
    return 0;
}
static FILE *wrappedStderr = &(FILE)FDEV_SETUP_STREAM(stderrWrapper, NULL, _FDEV_SETUP_WRITE);

#define EXPECT_NO_ERROR  \
    do {                 \
        errflag = false; \
    } while (0)

#define ASSERT_NO_ERROR                          \
    do {                                         \
        ATOMIC {                                 \
            if (errflag) {                       \
                TEST_FAILED("Unexpected error"); \
                HALT;                            \
            }                                    \
        }                                        \
    } while (0)

//! Global spawn counter
volatile uint16_t tt_spawns = 2;

//! converts a name of a scheduling strategy to a 4-digit initial
const char *tt_stratToName(SchedulingStrategy strat);
//! Tests if every strategy is able to spawn MAX_SPAWNS processes.
void tt_testSpawns(void);
//! Tests if killing a process will leave foreign critical sections
void tt_testForeignCriticalSections(void);

//! This programs includes the main procedure
REGISTER_AUTOSTART(program1)
void program1(void) {

    stderr = wrappedStderr;
    EXPECT_NO_ERROR;
    {

#if PHASE_1

        lcd_clear();
        lcd_writeProgString(PSTR("Phase 1: Foreign Crit. Sec."));
        delayMs(8 * DEFAULT_OUTPUT_DELAY);

        tt_testForeignCriticalSections();

#endif

#if PHASE_2

        lcd_clear();
        lcd_writeProgString(PSTR("Phase 2: Spawns"));
        delayMs(8 * DEFAULT_OUTPUT_DELAY);

        for (uint8_t run = 0; run < RUNS; run++) {
            tt_testSpawns();
        }

        // At this point every scheduling strategy was able to
        // spawn MAX_SPAWNS processes

#endif

        // kill everything else, show end and let idle take over
        os_enterCriticalSection();
        for (uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
            if (i != os_getCurrentProc()) {
                os_kill(i);
            }
        }
    }
    ASSERT_NO_ERROR;

// SUCCESS
#if CONFIRM_REQUIRED
    lcd_clear();
    lcd_writeProgString(PSTR("  PRESS ENTER!  "));
    os_waitForInput();
    os_waitForNoInput();
#endif
    TEST_PASSED;
    lcd_line2();
    lcd_writeProgString(PSTR(" WAIT FOR IDLE  "));
    delayMs(DEFAULT_OUTPUT_DELAY * 6);

    os_leaveCriticalSection();
}

void printProcesses() {
	os_enterCriticalSection();
	lcd_clear();
	lcd_line2();
	for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
		lcd_writeDec(os_getProcessSlot(i)->state);  // Schreibe die Zahl auf das LCD
	}
	os_leaveCriticalSection();
}

// We need 3 slots
void nonTerminatingProgram(void) {
    os_enterCriticalSection();
    tt_spawns++;
    os_leaveCriticalSection();
    while (1) {
        delayMs(DEFAULT_OUTPUT_DELAY);
    }
}

// The following program execute a new process of themselves and then die
void selfStartingProgram(void) {
    delayMs(DEFAULT_OUTPUT_DELAY);
    /*
     * The critical section makes sure, that we do not start a fork-bomb, e.g.
     * force termination of old process before first runtime for new process.
     * The dispatcher should clean this up.
     */
    os_enterCriticalSection();
    os_exec(selfStartingProgram, DEFAULT_PRIORITY);
    tt_spawns++;
}

/*!
 *  Tests whether the pid passed is invalid.
 *  If so, outputs a message and halts the program.
 *
 */
void tt_verifyPID(ProcessID pid) {
    if (pid < 1 || pid > 7) {
        ATOMIC {
            TEST_FAILED("os_exec failed: ");
            lcd_writeDec(pid);
            HALT;
        }
    }
}

/*!
 *  Tests if killing another process will leave foreign
 *  critical sections.
 */
void tt_testForeignCriticalSections(void) {
    os_enterCriticalSection();

    // Start a process and immediately kill it
    ProcessID pid = os_exec(nonTerminatingProgram, DEFAULT_PRIORITY);
    tt_verifyPID(pid);
    os_kill(pid);

    if (TIMSK2 & (1 << OCIE2A)) {
        ATOMIC {
            // Scheduler activated by os_kill()
            TEST_FAILED("Left crit. sec.");
            HALT;
        }
    }

    lcd_goto(2, 14);
    lcd_writeProgString(PSTR("OK"));
    delayMs(8 * DEFAULT_OUTPUT_DELAY);


    os_leaveCriticalSection();
}

/*!
 * Converts a name of a scheduling strategy to a 4-digit initials.
 * \param  strat Strategy of which you want to know the initials
 * \return       4-digit code for that strategy
 */
const char *tt_stratToName(SchedulingStrategy strat) {
    switch (strat) {
        case OS_SS_EVEN:
            return PSTR("EVEN");
        case OS_SS_INACTIVE_AGING:
            return PSTR("INAG");
        case OS_SS_RANDOM:
            return PSTR("RAND");
        case OS_SS_ROUND_ROBIN:
            return PSTR("RORO");
        case OS_SS_RUN_TO_COMPLETION:
            return PSTR(" RTC");
        default:
            return PSTR("NULL");
    }
}

static uint8_t tt_getNumberOfActiveProcs(void) {
    uint8_t num = 0;
    for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++) {
        if (os_getProcessSlot(pid)->state != OS_PS_UNUSED) num++;
    }
    return num;
}

/*!
 * Tests if every strategy is able to spawn MAX_SPAWNS processes.
 */
void tt_testSpawns(void) {
    // In this array you can select which strategies you want to test
    // Remove (or comment out) the strategies you don't want to test
    SchedulingStrategy strategies[] = {
        ////OS_SS_EVEN,
        //OS_SS_RANDOM,
        //OS_SS_ROUND_ROBIN,
        OS_SS_ROUND_ROBIN};
    const uint8_t numOfStrategies = 1;//sizeof(strategies) / sizeof(strategies[0]);
    uint8_t currentStrat = strategies[0];

    // This variable holds the process ID of the currently
    // running process of program 2
    ProcessID prog2ProcID = 0;

    // Start all remaining programs [MAX_NUMBER_OF_PROCESSES - (idle + program1 + program2 + free space)]
    for (uint8_t i = 3 + 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
        os_exec(selfStartingProgram, DEFAULT_PRIORITY);
    }

    for (uint8_t c = 0; c < numOfStrategies; c++) {
        currentStrat = strategies[c];
        os_setSchedulingStrategy(currentStrat);

        os_enterCriticalSection();
        tt_spawns = 0;
		printProcesses();
		lcd_clear();
		os_setStop();
		if(os_getStop() == 2){
			lcd_writeDec(getCritical());
			lcd_clear();
		}
        os_leaveCriticalSection();
		if(os_getStop() == 2){
			lcd_writeDec(getCritical());
			lcd_clear();
		}

        lcd_clear();

        // This loop runs until MAX_SPAWNS processes were spawned with the
        // selected strategy
        while (1) {
            lcd_line1();
            lcd_writeProgString(PSTR("Act.P.: "));
            lcd_writeDec(tt_getNumberOfActiveProcs());
            lcd_writeChar('/');
            lcd_writeDec(MAX_NUMBER_OF_PROCESSES);
            lcd_writeChar(' ');
            lcd_writeProgString(tt_stratToName(currentStrat));
            lcd_line2();
            lcd_writeProgString(PSTR("Spawns: "));
            os_enterCriticalSection();
            lcd_writeDec(tt_spawns);
            os_leaveCriticalSection();

            /*
             * Testing os_kill: If it works, a process of program 2 will be killed.
             * In the next iteration program 2 will be started again. If it does not
             * work, the process remains active and in the next iteration another
             * process of program 2 will be executed. Since program 2 does not
             * terminate on its own, spawning will cease after a few tries.
             */
            if (prog2ProcID) {
                os_kill(prog2ProcID);
                prog2ProcID = 0;
            } else {
                prog2ProcID = os_exec(nonTerminatingProgram, 1);
                tt_verifyPID(prog2ProcID);
            }

            // We want to break after a fixed number of spawns. Accessing a shared
            // variable must be safe though
            os_enterCriticalSection();
            uint16_t currentSpawns = tt_spawns;
            os_leaveCriticalSection();

            if (currentSpawns >= MAX_SPAWNS) {
                break; // out of spawn-counting loop
            }
        }
    }

    // Kill every test program
    for (uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		printProcesses();
        if (i != os_getCurrentProc()) {
            os_kill(i);
        }
    }
}
