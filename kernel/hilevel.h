/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


#include <string.h>

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the   kernel.

#include "lolevel.h"
#include     "int.h"

/* The kernel source code is made simpler and more consistent by using
 * some human-readable type definitions:
 *
 * - a type that captures a Process IDentifier (PID), which is really
 *   just an integer,
 * - an enumerated type that captures the status of a process, e.g.,
 *   whether it is currently executing,
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
 *   preservation and restoration prologue and epilogue, and
 * - a type that captures a process PCB.
 */

#define MAX_PROCS 100  // define max amounts of processes 
extern uint32_t tos_programs;  // top of stack of memory allocated to all  processes 

#define BASE_PRIORITY 1 // define  a base priority for each process. 
#define PROCESS_TERMINATED -1 // define a priority to be assign to terminated processes
typedef int pid_t;

typedef enum {
  STATUS_INVALID,
  STATUS_CREATED,
  STATUS_TERMINATED,

  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING
} status_t;


// execution context struct 
typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
  
} ctx_t;


// pcb table struct 
typedef struct {
     pid_t    pid; // Process IDentifier (PID)
  status_t status; // current status
  uint32_t    tos; // address of Top of Stack (ToS)
     ctx_t    ctx; // execution context
      int priority; // base priority 
     
      
     
} pcb_t;


 // status for the pipe , either open or closed 
typedef enum {
  STATUS_OPEN,
  STATUS_CLOSED
} pstatus_t;


// IPC STRUCT IMPLEMENTATION : PIPES

typedef struct {
	int fd[2]; // file descriptor of pipe , process write to  fd[1] and read from fd[0]
	pid_t start; //  process  id at the start of the pipe
	pid_t end; // process id  at the end of the pipe
  pstatus_t status; // pipe status
} pipe_t;

#endif
