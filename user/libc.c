/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

int  atoi( char* x        ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              : 
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}


void wait (){
  for(int i = 0; i<0x10000000; i++){
    asm volatile("nop");
  }
}


pid_t get_pid() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_GETPID
                "mov %0, r0 \n" // assign r0 = r
              : "=r" (r)
              : "I"  (SYS_GETPID)
              : "r0" );


  return ( pid_t ) r;
}

// Create a pipe ;
int make_pipe( int p0, int p1) {
	int r;
	asm volatile( "mov r0, %2 \n" // assign r0 =  p0
	              "mov r1, %3 \n" // assign r1 = p1
	              "svc %1     \n"  // make system call SYS_MAKEPIPE
	              "mov %0, r0 \n" //  assign r0 to r 
	              : "=r" (r)
	              : "I" (SYS_MAKEPIPE),"r" (p0), "r" (p1)
	              : "r0", "r1");
	return r;
}

// // write to pipe 
void write_to_pipe(int id, int buffer) {
     asm volatile( "mov r0, %1 \n" // assign r0 =  id
                   "mov r1, %2 \n" // assign r1 =    buffer
                   "svc %0     \n" // make system call SYS_WRITEPIPE
                   : 
                   : "I" (SYS_WRITEPIPE), "r" (id), "r" (buffer)
                   : "r0", "r1" );

      return;

}

//read from pipe 
int  read_from_pipe(int id){
    int r;
	asm volatile( "mov r0, %2 \n" // assign r0 =  id
	              "svc %1     \n"  // make system call SYS_READPIPE
	              "mov %0, r0 \n" //  assign r0 to r 
	              : "=r" (r)
	              : "I" (SYS_READPIPE),"r" (id)
	              : "r0" );
	return r;

}

//close pipe
int _close_pipe( int x ) {
  int r;
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %1     \n" // make system call SYS_EXIT
                "mov %0, r0 \n" // assign r0 = r  
              : "=r" (r)
              : "I" (SYS_CLOSEPIPE), "r" (x)
              : "r0" );

  return r;
}
