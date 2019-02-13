// File:	my_pthread.c
// Author:	Yujie REN
// Date:	January 2019

// List all group member's name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
ucontext_t parentContext;

threadQueue* threadQ=NULL;

void doNothing(){;}
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {

  printf("thread ID is? %d",thread);
	// Create Thread Control Block
  tcb* new_tcb=(tcb*) malloc(sizeof(tcb));
  new_tcb->priority=0;
  new_tcb->time_quantum_counter=0;
  new_tcb->threadId= thread;
  new_tcb->thread_status=READY;
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// After everything is all set, push this thread into run queue
  printf("\nWE CREATED A THREAD YALL\n");
  ucontext_t newThreadContext;
  getcontext(&newThreadContext);
  //where to return after function is done?
  newThreadContext.uc_link=0;
  //Define stack
  newThreadContext.uc_stack.ss_sp=malloc(STACK_SIZE);
  //Define stack size
  newThreadContext.uc_stack.ss_size=STACK_SIZE;
  //Set no flags
  newThreadContext.uc_stack.ss_flags=0;
  //Double check memory was allocated
  if (newThreadContext.uc_stack.ss_sp == 0 )
  {
         perror("Could not allocate space for thread");
         exit( 1 );
  }
  printf("Creating new Thread\n");
  //Make the context
  makecontext(&newThreadContext, function, 1, arg);
  new_tcb->context=newThreadContext;

  queueNode* qNode =(queueNode*) malloc(sizeof(queueNode*));
  qNode->thread_tcb=new_tcb;
  qNode->next=NULL;
  //Add context to TCB
  if(threadQ==NULL)
  {
    signal(SIGALRM, SIGALRM_Handler);//SIGALRM_Handler will call scheduler

    //setting timer to fire every TIME_QUANTUM milliseconds
    struct itimerval it_val;
    it_val.it_value.tv_sec =  TIME_QUANTUM/1000;
    it_val.it_value.tv_usec =  (TIME_QUANTUM*1000) % 1000000;
    it_val.it_interval = it_val.it_value;

    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
      perror("error calling setitimer()");
    }


    threadQ=(threadQueue*) malloc(sizeof(threadQueue));
    //initialize the head and tail
    threadQ->head=qNode;
    threadQ->tail=qNode;
    //threadQ->head->thread_tcb->thread_status=RUNNING;
    printf("Made the queue\n");
  }
  else{
    threadQ->tail->next=qNode;
    threadQ->tail=qNode;
  }

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// Switch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	// Waiting for a specific thread to terminate
	// Once this thread finishes,
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// Use the built-in test-and-set atomic function to test the mutex
	// If mutex is acquired successfuly, enter critical section
	// If acquiring mutex fails, push current thread into block list
	// and context switch to scheduler

	// YOUR CODE HERE
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in my_pthread_mutex_init

	return 0;
};


/* scheduler */
static void schedule() {
  //TODO: context switch to schedule function
  // Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

  //get the thread that was just running.
  queueNode* finishedThread=threadQ->head;
  //Double check the top of queue was running
  if(finishedThread->thread_tcb->thread_status==READY){
    printf("Top of queue was not running.\n");
    finishedThread->thread_tcb->thread_status=RUNNING;
    int swapStatus=swapcontext(&parentContext,&(finishedThread->thread_tcb->context));
    if(swapStatus!=0){
      printf("OOPSIES, Swap no work, error is: %d \nI'm exiting now\n",swapStatus);
      exit(0);

    }
    //exit(0);
  }
  else{
  //Change the status of the finished thread to ready
  finishedThread->thread_tcb->thread_status=READY;
  printf("just finished running Thread: %d\n",finishedThread->thread_tcb->threadId);
  //Move it to the back
  threadQ->tail->next=finishedThread;
  threadQ->tail=finishedThread;
  threadQ->head=threadQ->head->next;
  finishedThread->next=NULL;
  //Setup next thread to run
  queueNode* threadToRun=threadQ->head;
  threadToRun->thread_tcb->thread_status=RUNNING;
  //Swap context
  //Old Context (or this context??) to new context
  int swapStatus=swapcontext(&(finishedThread->thread_tcb->context),&(threadToRun->thread_tcb->context));
  if(swapStatus!=0){
    printf("OOPSIES, Swap no work, error is: %d \nI'm exiting now\n",swapStatus);
    exit(0);

  }
  //printf("\nswap staus should be 0:  %d\n",swapStatus);
}

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)
  printf("time to schedule my dude\n");
	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

void SIGALRM_Handler(){
  schedule();
}


/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE
