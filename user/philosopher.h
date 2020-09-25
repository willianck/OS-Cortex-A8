#ifndef _PHILOSOPHER_H
#define _PHILOSOPHER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



#include "libc.h"

#define MAX_PHILOSOPHERS 16


typedef enum { // Philosophers status , They can either be thinking hungry or eating
  STATUS_THINKING,
  STATUS_HUNGRY,
  STATUS_EATING,
} status_t;


typedef enum{ // philosophers can be in possesions of no fork,  a dirty fork or a clean fork  
    EMPTY,
    DIRTY,
    CLEAN,
} state_t;


// struct for the forks 
typedef struct{ 
    state_t status;   // give the status of the fork whether clean, dirty or empty 
    bool has_fork;  // state wether the philosopher is in possesion of the fork or not.
}  fork_t;



// struct that contains the philosophers 
typedef struct {
    status_t status;
    pid_t pid; // pid of the process running the philosopher 
    fork_t lfork; // left fork 
    fork_t rfork; // right fork 
    int lphilosopher; // left philosopher index
    int rphilosopher; // right philosopher index 
    bool request_tokenlf; /// a token hold by the philosopher to request a left fork from his left neighbour 
    bool request_tokenrf; // a token hold by the philosopher to request a  right fork from his right neighbour
} philo_t;



// type of message sent into file descriptor  for communication between philosophers

#define FORK_LTOKEN 4 // gives access to a left fork
#define FORK_RTOKEN 5 // gives access to a right fork 
#define REQ_LTOKEN 6 // request a left fork 
#define REQ_RTOKEN 7 // request a right fork 

#endif