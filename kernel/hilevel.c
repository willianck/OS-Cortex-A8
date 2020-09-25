/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
#include <stdio.h>
#include <unistd.h>


// A count that keeps track of the number of processes currently active in the pcb's
int running_processes = 0;

// Create a pcb table to hold all the processes 
pcb_t procTab[ MAX_PROCS ];

// create a process pointer that would point to the current process executing in the pcb table
 pcb_t* executing = NULL;

// create a table of pipes
pipe_t  pipes[MAX_PROCS];

// keeps track of the number of  pipes opened
int running_pipes;


// Mechanism that switches the execution context between the previous  and next process 
//  there by updating the process currently executing in the pcb table.  
void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
   
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    PL011_putc( UART0, prev_pid, true );
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    PL011_putc( UART0, next_pid, true );
    PL011_putc( UART0, ']',      true );
    PL011_putc( UART0, '\n', true );

    executing = next;                           // update   executing process to P_{next}

    if(prev->priority != -1){ // P_{prev} status is swithced to READY IF it is not terminated 
    prev->status = STATUS_READY; 
    }

    executing->status = STATUS_EXECUTING; // P_{next} status switched to executing
  return;

}

//updates the priority of processes not currently executing in the pcb table 
void  process_waiting_time() {
   for(int i= 0; i<running_processes; i++){
       // if process is not terminated and is not the one currently executing 
       // then increments process priority by 1
       if(procTab[i].status != STATUS_TERMINATED && (procTab[i].pid != executing->pid)){ 
               procTab[i].priority++;  
        }
   }
}


// returns the  maximum priority value  from the processes in the pcb table 
int max_priority() {
  int max = -1;
  for(int i = 0; i<running_processes; i++){
      if( (procTab[i].status != STATUS_TERMINATED)  && (procTab[i].priority > max)){
         max = procTab[i].priority;
      }
  }
  return max;
}

// return the process in the pcb table with maximum priority 
pid_t pop_pid(){
  for(int i=0; i<running_processes; i++){
      if(procTab[i].priority == max_priority()){
         return procTab[i].pid;
      }
  }
}


/* A Priorty-Based Scheduler which schedules the processes based on their priority. 
   The process with highest priority will be the next process to be executed 
   Given the current executing context, the scheduler gets the next process to be executed and calls dispatch */
 void pbScheduler( ctx_t* ctx ) {

   // A kernel interrupt or syscall  has occured  , a context switch is about to happen
   // update the priorities for each process in the pcb
   process_waiting_time();
  pid_t pid = pop_pid(); // pop process with highest priority 
  pcb_t* prev = executing; // executing process now becomes the previous process 
  dispatch(ctx, prev, &procTab[pid]);  // call the dispatcher  to make the context swtich 

}

// returns the next process ID identifier to be used in the pcb table  
pid_t next_pid() {
  int x = -1;
// loops through pcb table and find the first entry with an invalid or terminated status 
  for ( int i = 0; i < MAX_PROCS; i++ ) {
    if ( procTab[i].status == STATUS_TERMINATED || procTab[i].status == STATUS_INVALID ) {
      x = i;
      break;
    }
  }
  return x; // Returns -1 if there is no space available in the pcb table 
}


extern void    main_console();  // address to console's program entry point 



 //---------------------------------------------------- IPC HELPER FUNCTIONS 

 // returns the Identifier of the next pipe available.
 // if no pipe is available return -1
int next_pipeid(){
    for ( int i = 0; i < MAX_PROCS; i++ ) {
		    if ( pipes[ i ].end == 0 ) return i;
	  }
	return -1;

}



// Initialise the pipe and returns its identifier
int init_pipe(pid_t p0 , pid_t p1){
     int id =  next_pipeid(); // get the next pipe id 
     if(id > -1){  // Checks that the id is valid 
       pipes[ id ].start = p0;
		   pipes[ id ].end = p1;
		   pipes[ id ].fd[0] = -1; // set  the fd  for reading to -1
		   pipes[ id ].fd[1] = -1; // set the fd  for  writing to -1
       pipes[ id ].status = STATUS_OPEN;
	   } 
     else{
        return -1; // invalid id hence can not  initialise tbe pipe
     }
     return id; // 
     
}

//  closes pipe when done 
int close_pipe(int id){
   pipes[id].status = STATUS_CLOSED;
   memset( &pipes[ id ], 0, sizeof( pipes ) );
  
}

//------------------------------------------------------------------------------------------------

void hilevel_handler_rst(ctx_t* ctx) {

    TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
    TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
    TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
    TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
    TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

    GICC0->PMR          = 0x000000F0; // unmask all            interrupts
    GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
    GICC0->CTLR         = 0x00000001; // enable GIC interface
    GICD0->CTLR         = 0x00000001; // enable GIC distributor

  

  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;    // make all processes in the pcb table invalid 
    memset( &pipes[ i ], 0, sizeof( pipes ) ); // memset our pipes table 
  }


  
  

  /*The Console automatically execute the user programs P3 , P4 and P5 and any other process  when they are called
  by setting their fields in the pcb table appropriately
   *  Note in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR mode,
   *   with IRQ interrupts enabled, and
   * - the PC and SP values match the entry point and top of stack.
   */


   /* The Console process  is initialise in the pcb table . All other processes stems from 
     the console creating them as child processes via the fork sys call and then executed
    through the exec sys call. This is done via the console command line where the user can 
    type in the available commands */
   
  memset( &procTab[ 0 ], 0, sizeof( pcb_t ) ); // initialise 0-th PCB = P_0 
  procTab[ 0 ].pid      = 0;
  procTab[ 0 ].status   = STATUS_READY;
  procTab[ 0 ].tos      = ( uint32_t )( &tos_programs ); 
  procTab[ 0 ].ctx.cpsr = 0x50;
  procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_console ); // points to  main_console() , the program entry point 
  procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
  procTab[ 0 ].priority = BASE_PRIORITY;
  running_processes++; // one  process has been initialised so the number of processes is incremented 




  /* Once the PCBs are initialised, we arbitrarily select the 0-th PCB to be
   * executed: there is no need to preserve the execution context, since it
   * is invalid on reset (i.e., no process was previously executing).
   */
   
   
  dispatch(ctx, NULL, &procTab[0]);
  int_enable_irq();
  return;
}




void hilevel_handler_irq(ctx_t* ctx ) {
  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) { 
    pbScheduler(ctx); // after an interrupt , call  the scheduler to get  the next process running 
    PL011_putc( UART0, 'T', true ); TIMER0->Timer1IntClr = 0x01;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;
 
  return;
}

void hilevel_handler_svc(ctx_t* ctx, uint32_t id) {
  /* Based on the identifier (i.e., the immediate operand) extracted from the
 * svc instruction,
 *
 * - read  the arguments from preserved usr mode registers,
 * - perform whatever is appropriate for this system call, then
 * - write any return value back to preserved usr mode registers.
 */

switch( id ) {
  case 0x00 : { // 0x00 => yield()  // process gives control over to the kernel 
    pbScheduler( ctx ); 

    break;
  }

  case 0x01 : { // 0x01 => write( fd, x, n )  
    int   fd = ( int   )( ctx->gpr[ 0 ] );
    char*  x = ( char* )( ctx->gpr[ 1 ] );
    int    n = ( int   )( ctx->gpr[ 2 ] );

    for( int i = 0; i < n; i++ ) {
      PL011_putc( UART0, *x++, true );
    }

    ctx->gpr[ 0 ] = n;

    break;
  }

  case 0x03  : { // 0x032 => fork() // 
        PL011_putc( UART0, '[', true );
        PL011_putc( UART0, 'F', true );
        PL011_putc( UART0, 'O', true );
        PL011_putc( UART0, 'R', true );
        PL011_putc( UART0, 'K', true );
        PL011_putc( UART0, ']', true );
        PL011_putc( UART0, '\n', true );
        
        pid_t  pidcp =  next_pid(); // get the new pid for process child; 
        if( pidcp == -1) { // no space available to fork new child 
                break;
         }
        pid_t pidpp = executing->pid; // pid of current process
        

         // initialise our new child process in the pcb table
        memset( &procTab[ pidcp], 0, sizeof( pcb_t ) ); 
        memcpy(&procTab[pidcp].ctx,ctx, sizeof(ctx_t));
        procTab[ pidcp].pid      = pidcp;
        procTab[ pidcp].status   = STATUS_CREATED;
        procTab[ pidcp ].tos     =  (uint32_t) (&tos_programs) - (pidcp * 0x00001000);  // each process is allocated a space of 0x00001000
        procTab[ pidcp ].ctx.cpsr = procTab[pidpp].ctx.cpsr; 
        uint32_t instruction_offset =  (uint32_t) (&executing->tos - &ctx->sp);
        procTab[ pidcp].ctx.sp   = (uint32_t)( &procTab[pidcp].tos) - instruction_offset;
        procTab[pidcp].priority = BASE_PRIORITY;

       //return child's pid to parent process  and zero to  the child  process as a way to identify them.
        ctx->gpr[0] = pidcp;
        procTab[pidcp].ctx.gpr[0] = 0;  
        procTab[pidcp].status = STATUS_READY; // process is ready 
        running_processes++; // increment number of running  processes


    break;
  }


  case 0x05 : { // 0x05  => exec() 
      PL011_putc( UART0, '[', true );
      PL011_putc( UART0, 'E', true );
      PL011_putc( UART0, 'X', true );
      PL011_putc( UART0, 'E', true );
      PL011_putc( UART0, 'C', true );
      PL011_putc( UART0, ']', true );
      PL011_putc( UART0, '\n', true );
    ctx->sp = executing->tos;  //sp now matches the tos of the  process 
    ctx->pc  = ctx->gpr[0];   // and pc point to the instruction address to exec new program
    
    break;
  }


  case 0x04 : { // 0x04 => exit()
    PL011_putc( UART0, '[', true );
      PL011_putc( UART0, 'E', true );
      PL011_putc( UART0, 'X', true );
      PL011_putc( UART0, 'I', true );
      PL011_putc( UART0, 'T', true );
      PL011_putc( UART0, ']', true );
      PL011_putc( UART0, '\n', true );
   if( ctx->gpr[0] == EXIT_SUCCESS){
    executing->priority = PROCESS_TERMINATED; // set negative priority to declare that process is terminated 
    executing->status  = STATUS_TERMINATED;
    pbScheduler(ctx); // call the scheduler to get the next process 
   }
    break;
  }

  case 0x08 : {  // 0x08 => get_pid()
      ctx->gpr[ 0 ] = executing->pid; // set return value to pid of current process
      break;
    }

  case 0x07 : { // 0x07 => nice()
     PL011_putc( UART0, '[', true );
      PL011_putc( UART0, 'N', true );
      PL011_putc( UART0, 'I', true );
      PL011_putc( UART0, 'C', true );
      PL011_putc( UART0, 'E', true );
      PL011_putc( UART0, ']', true );
      PL011_putc( UART0, '\n', true );
      procTab[ctx->gpr[0]].priority = ctx->gpr[1]; 
      pbScheduler(ctx);
    break;
  }


  case 0x09: { // 0x09 => make_pipe(pid_t p0, pid_t p1)
                int start  = ctx->gpr[0]; // start process of pipe 
		            int end    = ctx->gpr[1]; // end process of pipe 
		            int pipe_id = init_pipe(start, end); // initialise the pipe 
		            running_pipes ++; 
		            ctx -> gpr[0] = pipe_id;  // returns pipe's  id  so that  we can identify it in the ipc table 
           break;
  }  
  
  case 0x10: { // 0x10 => write_to_pipe(int id, int buffer)
             PL011_putc( UART0, '[', true );
             PL011_putc( UART0, 'W', true );
             PL011_putc( UART0, 'R', true );
             PL011_putc( UART0, 'I', true );
             PL011_putc( UART0, 'T', true );
             PL011_putc( UART0, 'E', true );
             PL011_putc( UART0, ']', true );
             PL011_putc( UART0, '\n', true );
             int id = ctx->gpr[0];  // get the pipe's id 
             int buffer = ctx->gpr[1]; // copy the data to be communicated 
             pipe_t* pipe = &pipes[ id ];
             pipe->fd[1] = buffer; // set the file descriptor fd[1] to the buffer 
             
  break;
}



  case 0x11 : { // 0x11 => read_from_pipe(int id)
             PL011_putc( UART0, '[', true );
             PL011_putc( UART0, 'R', true );
             PL011_putc( UART0, 'E', true );
             PL011_putc( UART0, 'A', true );
             PL011_putc( UART0, 'D', true );
             PL011_putc( UART0, ']', true );
             PL011_putc( UART0, '\n', true );
             int id = ctx->gpr[0]; // get the pipe's id 
             pipe_t* pipe = &pipes[ id ]; 
             pipe->fd[0] = pipe->fd[1];     // file descriptor fd[0] reads from fd[1]
             ctx->gpr[0] = pipe->fd[0]; // returns read value to  the executing process
       
  break;
}
 

  case 0x12 : { // 0x12 => _close_pipe(int x)
      int id  = ctx->gpr[0];    
      if( pipes[id].start == executing->pid){ // check that start process is the one closing the pipe 
          close_pipe(id);
      }
      else{ 
          ctx->gpr[0] = -1; // not start process so can not close pipe 
      } 
  break;
  }

}

  return;
}
