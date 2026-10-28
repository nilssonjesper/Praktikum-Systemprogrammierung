/*! \file
 *  \brief TaskManager library for the OS.
 *
 *  Contains the user controlled task manager functionality.
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#ifndef _OS_TASKMAN_H
#define _OS_TASKMAN_H

#include <stdbool.h>

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------

//! Main loop for the TaskManager
void os_taskManMain(void);

//! Returns true if the TaskManager is currently open
bool os_taskManOpen(void);

#endif
