#include "philosopher.h"


 philo_t  philosophers[MAX_PHILOSOPHERS]; // a table of philosophers 


int chans[MAX_PHILOSOPHERS]; // table containing index to each pipes  for example chans[0] => pipe[0] => (0--1)


// get the left  philosopher 
int left(int x){
    int y = x-1;
    if( y== -1) return 15;
    else return y;
}

// get the right  philosopher 
int right(int x){
    int y = x+1;
    if(y==16) return 0;
    else return y;

}

// set up the pipes for the philosophers
void set_pipes(){
    for(int i =0; i<MAX_PHILOSOPHERS; i++){
        if(i<15){
        int id = make_pipe(i,(i+1));
        chans[i] = id;
        }
        else{
        int id = make_pipe(i,0);
        chans[i] = id;    
        }
    }
}


void init_table(int id){ // Initialise the dining table 
        philosophers[id].status = STATUS_THINKING;
    if(id==0){ // first philosopher 
        philosophers[id].lfork.status = DIRTY;
        philosophers[id].lfork.has_fork = true;
        philosophers[id].rfork.status = DIRTY;
        philosophers[id].rfork.has_fork = true;
        philosophers[id].lphilosopher = left(id);
        philosophers[id].rphilosopher = right(id);
        philosophers[id].request_tokenlf = false; // no request token for left fork;
        philosophers[id].request_tokenrf = false; // no request token for right fork;

    }
    else if(id ==  15){ // last philosopher 
         philosophers[id].lfork.has_fork = false; // philosopher 15 has no left fork
         philosophers[id].rfork.has_fork = false; // philosopher 15 has no right fork
         philosophers[id].lfork.status = EMPTY;
         philosophers[id].rfork.status = EMPTY; 
         philosophers[id].lphilosopher = left(id);
         philosophers[id].rphilosopher = right(id); 
         philosophers[id].request_tokenlf = true; // request token for lfork
         philosophers[id].request_tokenrf = true; // request toekn for rfork
        
    }
    else {
        philosophers[id].lfork.has_fork = false; // remaining philosophers have no left fork
        philosophers[id].rfork.has_fork = true;  
        philosophers[id].lfork.status =  EMPTY;
        philosophers[id].rfork.status = DIRTY;
        philosophers[id].lphilosopher = left(id);
        philosophers[id].rphilosopher = right(id);
        philosophers[id].request_tokenlf = true; //  request token for lfork
        philosophers[id].request_tokenrf = false; // no request token for rfork
     

    } 

    
}



// makes the philosopher think for some time then changes his status to hungry
void think(char* c, int id){
     write( STDOUT_FILENO, "Philosopher ", 13);
     write( STDOUT_FILENO, c, 2);
     write( STDOUT_FILENO, " IS THINKING\n", 14 );
     wait();
     //finished thinking,  philosopher is now Hungry   
     philosophers[id].status = STATUS_HUNGRY;
     
}


// philosophers finished eating  and his forks are set to dirty 
void done(int id){
   philosophers[id].rfork.status = DIRTY;
   philosophers[id].lfork.status = DIRTY;

}

// philosopher eats for some times then changes his status to thinking
void eat(char* c,int id){
    philo_t* p = &philosophers[id];
    p->status = STATUS_EATING;
    write( STDOUT_FILENO, "Philosopher  ", 14);
    write( STDOUT_FILENO, c, 3);
    write( STDOUT_FILENO, " IS EATING !\n", 14 );
    wait(); 
    //finish eating philosopher is now thinking
    done(id);
    philosophers[id].status = STATUS_THINKING;
    write( STDOUT_FILENO, "Philosopher  ", 14);
    write( STDOUT_FILENO, c, 3);
    write(STDOUT_FILENO," HAS fINISHED EATING!\n",23);
    
}



/* After receiving a left fork , a philosopher checks if he can eat provided he has two forks AND
 either has no request for his left fork or it is clean  */
bool can_eat_leftcheck(int id){
      philo_t* p = &philosophers[id];
       return  (p->status == STATUS_HUNGRY) && (p->lfork.has_fork) && (p->rfork.has_fork) &&
         ((!p->request_tokenrf) ||  (p->lfork.status == CLEAN)); 
}


/* After receiving a right fork , a  philosopher checks if he can eat provided he has two forks AND
either has no request for his right fork or it is clean */
bool can_eat_rightcheck(int id){
     philo_t* p = &philosophers[id];
      return  (p->status == STATUS_HUNGRY) && (p->lfork.has_fork) && (p->rfork.has_fork) &&
         ((!p->request_tokenlf) ||  (p->rfork.status == CLEAN));         
      }


//Given that  a philosopher receives a  left fork token , he now has the fork and it is clean 
void receive_lfork(int id){
    philo_t* p = &philosophers[id];
    int buffer = read_from_pipe(chans[p->lphilosopher]); // read on previous channel 
    if(buffer == FORK_LTOKEN){ //Received left fork 
            philosophers[id].lfork.has_fork = true;
            philosophers[id].lfork.status  = CLEAN;
    }  
}


//Given that  a philosopher receives a  right fork token , he now has the fork and it is clean 
void receive_rfork(int id){
  philo_t* p = &philosophers[id];
  int  buffer = read_from_pipe(chans[id]); // read on current channel 
  if(buffer== FORK_RTOKEN){ // received  right fork
            philosophers[id].rfork.has_fork = true;
            philosophers[id].rfork.status = CLEAN;
        
    }
}


// the left Philosopher receives the left fork request from the current philosopher  so he is now able to satisfy that request 
void receive_lreq(int id){
     philo_t* p = &philosophers[id];
      int buffer = read_from_pipe(chans[id]);  // read on current channel 
      if(buffer == REQ_LTOKEN){ // receive left token 
            philosophers[id].request_tokenlf = true;
        }
}

// the right Philosopher receives the right  fork request from the current philosopher  so he is now able to satisfy that request 
void receive_rreq(int id){
     philo_t* p = &philosophers[id];
     int buffer = read_from_pipe(chans[p->lphilosopher]); // read on prev channel
     if(buffer == REQ_RTOKEN){ // receive right token 
            philosophers[id].request_tokenrf = true;  
        }
}



// given that a philosopher is hungry he can request a  left fork if he has the token for it 
void request_lfork(int id){
    philo_t* p = &philosophers[id];
    if(p->status == STATUS_HUNGRY && (p->request_tokenlf) && (!p->lfork.has_fork) ){ 
        write_to_pipe(chans[p->lphilosopher],REQ_LTOKEN); // send left fork request token to left philosopher 
        p->request_tokenlf = false;  
    }

}

// given that a philosopher is hungry he can request a right fork if he has the token for it
void  request_rfork(int id){
       philo_t* p = &philosophers[id];
       if(p->status ==  STATUS_HUNGRY && (p->request_tokenrf) && (!p->rfork.has_fork)){ 
           write_to_pipe(chans[id],REQ_RTOKEN); // send right fork request token to right philosopher
           p->request_tokenrf = false;
               
       }

}



// left philosopher releases right fork since it has been requested by the current philosopher  who needed a left fork 
void release_rfork(int id){
       philo_t* p = &philosophers[id];
       if(p->status != STATUS_EATING && (p->request_tokenlf) && p->rfork.status == DIRTY){
           write_to_pipe(chans[id],FORK_LTOKEN);  // send the right fork to right neighbour who requested a left fork 
           // philosopher is not in possesion of the forks any longer 
           p->rfork.has_fork = false;
           p->rfork.status = EMPTY;   
       }
}


// right philosopher releases left  fork since it has been requested by current philosopher who needed a right fork 
void release_lfork(int id){ 
       philo_t* p = &philosophers[id];
       if(p->status != STATUS_EATING && (p->request_tokenrf) && p->lfork.status == DIRTY){
           write_to_pipe(chans[p->lphilosopher],FORK_RTOKEN); // sends the left fork to left neighbour who requested a right fork 
           // philosopher is not in possesion of the fork any longer 
           p->lfork.has_fork = false;
           p->lfork.status = EMPTY;   
       }
}


//----------------------------------------------------------------------------------------------------------------------------------------------------
// Main entry point of the program 

void main_philosopher(){
write(1,"The dining philosophers are ready\n",35);
 set_pipes();  // set the pipes for philosophers 

 for(int i = 0; i<MAX_PHILOSOPHERS; i++){
     init_table(i); // initialise the dinner table 
 }

 for(int i =0; i<MAX_PHILOSOPHERS; i++){
      if(fork()==0){
            int cpid = get_pid();
            philosophers[i].pid = cpid; // capture child's pid in philosopher 
            char c[3];
            itoa(c,cpid);
            while(1){
                  think(c,i);  //  philosopher is thinking
                  request_lfork(i); //  philosopher request left fork 
                  release_rfork(left(i)); // left philosopher releases right fork 
                  receive_lreq(left(i)); // left philosopher receives left fork request 
                  receive_lfork(i); // philosopher receives left fork 
                  if(can_eat_leftcheck(i)){ // philosopher makes an attempt to eat 
                  eat(c,i); 
                  yield(); // philosopher has eaten , he yields control over to give the chance to other  
                  }        // philosophers to eat 
               
                 else {
                  request_rfork(i); // philosopher request right fork 
                  release_lfork(right(i)); // right philosopher releases left fork 
                  receive_rreq(right(i)); // right philosopher receives right fork request 
                  receive_rfork(i); // philosopher receives right fork 
                  if(can_eat_rightcheck(i)){  // philosopher makes an attempt to eat 
                     eat(c,i);
                  } 
                 } 
            }
       }
   }

  exit(EXIT_SUCCESS);  
}
















