// File:	my_pthread.c
// Author:	Yujie REN
// Date:	January 2019

// List all group member's name:
// username of iLab:
// iLab Server:
//
#include "my_pthread_t.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
//ucontext_t scheduleContext=NULL;
ucontext_t schedulerContext;
ucontext_t parentContext;
threadCounter=0;
threadQueue* threadQ=NULL;
//ucontext_t processFinishedJobContext;

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
  *thread=++threadCounter; //???????????????????????????????????????????????????????????????????????????????????
  //First time we create a threadQ
  //Initialize Scheduler Context
  if(threadQ==NULL)
  {
    getcontext(&schedulerContext);
    //where to return after function is done?
    schedulerContext.uc_link=0;
    //Define stack
    schedulerContext.uc_stack.ss_sp=malloc(STACK_SIZE);
    //Define stack size
    schedulerContext.uc_stack.ss_size=STACK_SIZE;
    //Set no flags
    schedulerContext.uc_stack.ss_flags=0;
    //Double check memory was allocated
    if (schedulerContext.uc_stack.ss_sp == 0 )
    {
           perror("Could not allocate space for return context");
           exit( 1 );
    }
    makecontext(&schedulerContext, (void*)&schedule, 0);
  }
   //BLOCK TIMER FROM INTERRPTING
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGALRM);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);


  printf("thread ID is? %d",*thread);
	// Create Thread Control Block
  tcb* new_tcb=(tcb*) malloc(sizeof(tcb));
  new_tcb->priority=0;
  new_tcb->time_quantum_counter=0;
  new_tcb->threadId = *thread;
  new_tcb->thread_status=READY;
  new_tcb->join_boolean=0; //FALSE
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// After everything is all set, push this thread into run queue

  //Experiment with finished Job context
  //ucontext_t threadReturnContext;
  getcontext(&(new_tcb->return_context));
  //does a setContext to this context when done
  (new_tcb->return_context).uc_link=&schedulerContext;
  //Define stack
  (new_tcb->return_context).uc_stack.ss_sp=malloc(STACK_SIZE);
  //Define stack size
  (new_tcb->return_context).uc_stack.ss_size=STACK_SIZE;
  //Set no flags
  (new_tcb->return_context).uc_stack.ss_flags=0;
  //Double check memory was allocated
  if ((new_tcb->return_context).uc_stack.ss_sp == 0 )
  {
         perror("Could not allocate space for return context");
         exit( 1 );
  }
  makecontext(&(new_tcb->return_context), (void*)&processFinishedJob, 1, new_tcb->threadId);
  //makecontext(&processFinishedJobContext, (void*)&processFinishedJob, 0);


  printf("\nWE CREATED A THREAD (%d) YALL\n", *thread);
  ucontext_t newThreadContext;
  getcontext(&newThreadContext);
  //where to return after function is done?
  newThreadContext.uc_link=&(new_tcb->return_context);
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
  makecontext(&newThreadContext, (void*)function, 1, arg);
  // makecontext(&newThreadContext, function, 1, arg);
  new_tcb->context=newThreadContext;

  queueNode* qNode =(queueNode*) malloc(sizeof(queueNode*));
  qNode->thread_tcb=new_tcb;
  qNode->next=NULL;


  signal(SIGALRM, SIGALRM_Handler);//SIGALRM_Handler will call scheduler
  start_timer(TIME_QUANTUM);//setting timer to fire every TIME_QUANTUM milliseconds

  if(threadQ==NULL)
  {
    threadQ=(threadQueue*) malloc(sizeof(threadQueue));
    //initialize the head and tail
    threadQ->head=qNode;
    threadQ->tail=qNode;
    printf("Made the queue\n");
  }
  else{
    threadQ->tail->next=qNode;
    threadQ->tail=qNode;
  }
  //ALLOW TIMER TO CONTINUE
  sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// Switch from thread context to scheduler context

	// YOUR CODE HERE
  SIGALRM_Handler();
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
  //getTopOfQueue
  queueNode* finishedQNode=threadQ->head;//getTopOfQueue();
  tcb* finishedThread=finishedQNode->thread_tcb;
  //Set to done
  printf("\nA job just decided to exit with ID %d \n",finishedThread->threadId);
  finishedThread->thread_status=DONE;
  SIGALRM_Handler();

}


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {

  printf("\n\n\n\n\n******************JOIN (%d)**************\n", thread);
	// Waiting for a specific thread to terminate
	// Once this thread finishes,
	// Deallocated any dynamic memory created when starting this thread
	// YOUR CODE HERE
  //TODO: returning Value_ptr

  //First, check if thread is in the queueNode
  tcb* thread_to_join=findThread(thread);
  //If thread is in queue, check its status
    //If thread status is Done, free it, and swap context with Parent
    //If thread status is READY or RUNNING, yield CPU
  //If thread is not in queueNode, set context to parent
  if(thread_to_join==NULL){
    printf("Thread to join (%d) on is no longer in queue\n",thread);
    /*
    int setStatus=setcontext(&parentContext);
    if(setStatus){
      printf("Error setting context, exiting\n");
      exit(1);
    }
    */
    //TODO: Return value_ptr if not nULL
    return 0;
  }
  //Thread is in Queue
  else{
    //If thread is Not done, yield CPU
    if(thread_to_join->thread_status!=DONE){
      printf("thread to join (%d) is still executing, yielding CPU\n",thread);
      thread_to_join->join_boolean = 1;
      // SWAP TO SCHEDULER schedule()???? swapcontext()????
      //schedule();
      SIGALRM_Handler();
      printf("Done yielding the CPU\n");
      return 0;
      //my_pthread_yield();
      //WHAT DO WE DO HERE???
    }
    //If thread is done, free and return? or setContext to Main?
    else{
      printf("thread to join (%d) is done\n",thread);
      free(thread_to_join->context.uc_stack.ss_sp);
      free(thread_to_join->return_context.uc_stack.ss_sp);
      free(thread_to_join);
      //TODO: Remove thread from Queue
      //TODO: Return value_ptr if not nULL
      return 0;
  }
  printf("Shouldn't reach here (end of join)\n");
	return 0;
}
}

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
  printf("yo I am gonna like totally destroy you (a mutex) now\n");
	return 0;
};

struct timespec timeCheck;
int firstSchedule=1;
/* scheduler */
static void schedule() {
  //TODO: context switch to schedule function
  // Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function
  //printf("scheduling\n");
  struct timespec prev_time=timeCheck;
  clock_gettime(CLOCK_REALTIME, &timeCheck);
  if(!firstSchedule){
    printf("scheduling time: %lu milli-seconds\n", (timeCheck.tv_sec - prev_time.tv_sec) * 1000 + (timeCheck.tv_nsec - prev_time.tv_nsec) / 1000000);
  }
  else{
    printf("first schedule\n");
    firstSchedule=0;

  }
  //get the thread that was just running.
  // queueNode* finishedThread=threadQ->head;
  queueNode* finishedThread=getTopOfQueue();
  //No jobs in queue
  if(finishedThread==NULL)
  {
    //printf("No jobs in queue\n");
    //TODO: Set main
    printf("we already switched to main, this should never print\n");
    setcontext(&parentContext);

  }
  //Check if top of queue is ready
  if(finishedThread->thread_tcb->thread_status==READY){
    printf("Top of queue was not running (ready).\n");
    finishedThread->thread_tcb->thread_status=RUNNING;
    //Finished thread never actuall ran, so run it.
    // int setStatus=setcontext(&(finishedThread->thread_tcb->context));

    //TODO: Make sure main was saved in sigalarm
    int setStatus=setcontext(&(finishedThread->thread_tcb->context));
    if(setStatus!=0){
      printf("\nOOPSIES, Swap no work, error is: %d \nI'm exiting now\n",setStatus);
      exit(0);
    }
    printf("should never reach here 1.\n");
    //exit(0);
  }
  //Top of queue has finished executing
  else if(finishedThread->thread_tcb->thread_status==DONE)
  {
    printf("thread is done, removing\n");

    // threadQ->head=finishedThread->next; // removing
    removeFromQueue(finishedThread);
    // queueNode* threadToRun=threadQ->head; // get next to run
    queueNode* threadToRun=getNextToRun(); // get next to run
    if(finishedThread->thread_tcb->join_boolean==1){
      //swap to main
      printf("THREAD (%d) to join is DONE, returning to main\n", finishedThread->thread_tcb->threadId);
      // free(finishedThread); ADD THIS
      setcontext(&parentContext);
    }
    // free(finishedThread); ADD THIS
    if(threadToRun==NULL)
    {
      printf("No more threads to run\n");
      setcontext(&parentContext); // done executing everything in Q, switching back to main
      //continue;
    }
    else{
      threadToRun->thread_tcb->thread_status=RUNNING;
      //Swap context
      //Set context starts from the top
      int setStatus=setcontext(&(threadToRun->thread_tcb->context));
      //int setStatus=swapcontext(&parentContext,&(threadToRun->thread_tcb->context));
      if(setStatus!=0){
        printf("OOPSIES, Set no work, error is: %d \nI'm exiting now\n",setStatus);
        exit(0);

      }
    }

  }
  else{
  //Change the status of the finished thread to ready
  finishedThread->thread_tcb->thread_status=READY;
  printf("about to swap out running Thread: %d\n",(finishedThread->thread_tcb->threadId));
  //Move it to the back
  threadQ->tail->next=finishedThread;
  threadQ->tail=finishedThread;
  threadQ->head=threadQ->head->next;
  finishedThread->next=NULL;
  //Setup next thread to run
  // queueNode* threadToRun=threadQ->head;
  queueNode* threadToRun=getNextToRun();
  threadToRun->thread_tcb->thread_status=RUNNING;
  //Swap context saves lace in old context for later
  //int setStatus=swapcontext(&(finishedThread->thread_tcb->context),&(threadToRun->thread_tcb->context));
  int setStatus=setcontext(&(threadToRun->thread_tcb->context));
  if(setStatus!=0){
    printf("OOPSIES, Swap no work, error is: %d \nI'm exiting now\n",setStatus);
    exit(0);
  }
  //printf("\nswap staus should be 0:  %d\n",swapStatus);
}

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)
  printf("end of scheduler, should not be here,\n");
  printf("Setting context back to main\n");
  setcontext(&parentContext);
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
  //TODO: Check if parentContext is the one who was interrupted. Could be thread or Main
  //swapcontext(&parentContext,&schedulerContext);

  //schedule();
  //printf("Ok resumin now\n");
  //get the thread that was just running.
  //printf("Interrupted!\n");
  queueNode* finishedThread= getTopOfQueue();
    //No jobs in queue
  if(finishedThread==NULL)
  {
    //printf("No jobs in queue\n");
    //printf("\nInterrupted from Main, no jobs left in queue, going back to main\n");
    // setcontext(&parentContext);
    return;

  }
  //Check if top of queue is ready
  if(finishedThread->thread_tcb->thread_status==READY){
    printf("\nInterrupted from Main, switching to schedule context\n");
    swapcontext(&parentContext,&schedulerContext);
    printf("Resuming Main\n");
  }
  //top of queue was Running or Done
  else{
    printf("\ninterrupted from thread %d\n",(threadQ->head->thread_tcb->threadId));
    swapcontext(&(threadQ->head->thread_tcb->context),&schedulerContext);
    printf("resuming thread %d\n",(threadQ->head->thread_tcb->threadId));
  }

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
//Marks finished Threads as DONE
void processFinishedJob(int threadID){
  printf("\nA job just finished!!!! with ID %d (This is so good :) \n",threadID);
  tcb* finishedThread=findThread(threadID);
  printf("Found thread!\n");
  finishedThread->thread_status=DONE;
  free(finishedThread->context.uc_stack.ss_sp);
  SIGALRM_Handler();
}

/*Search for a thread by its threadID*/
tcb* findThread(int threadID){

  if(threadQ == NULL)
  {
    printf("Queue is Null\n");
    return NULL;
  }
  //Linear search through Queue for threadID
  queueNode* head=threadQ->head;
  printf("about to search list for thread %d\n",threadID);
  if(head==NULL){
    printf("head was NULL while searching for thread # (%d)\n", threadID);
    return NULL;
  }
  if((int)(head->thread_tcb->threadId)==(int)(threadID)){
    printf("found as head\n");
    return head->thread_tcb;
  }
  while(head!=NULL && (int)(head->thread_tcb->threadId)!=(int)(threadID) ){
    printf("ID: %d\n",head->thread_tcb->threadId);
    if((int)(head->thread_tcb->threadId)==(int)(threadID)){
      return head->thread_tcb;
    }
    head=head->next;
  }

  //Reached end of list
  if(head==NULL)
  {
    printf("Thread (%d) not found.\n", threadID);
    return NULL;
  }
  //Thread found
  printf("found thread? %d\n",head->thread_tcb->threadId);
  return head->thread_tcb;
}

void start_timer(int timeQ){// starts a timer to fire every timeQ milliseconds
  struct itimerval it_val;
  it_val.it_value.tv_sec =  timeQ/1000;
  it_val.it_value.tv_usec =  (TIME_QUANTUM*1000) % 1000000;
  it_val.it_interval = it_val.it_value;

  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    perror("error calling setitimer()");
  }
}

queueNode* getTopOfQueue(){ // returns top of queue according to current scheduling paradigm
  if(threadQ==NULL){
    printf("Queue is NULL!!!\n");
    return NULL;
  }
  queueNode* head=threadQ->head;

  if(head==NULL){
    printf("Head is NULL\n");
    return NULL;
  }
  else{
    return head;
  }
  // ifndef MLFQ
  // MLFQ top of queue
}

void removeFromQueue(queueNode *finishedThread){
  //Gonna have to change this logic
  // CURRENTLY this function removes whatever is the HEAD of the Q
  threadQ->head=finishedThread->next;
  return;
  //TODO: implement other types of removing from Q's and multi level Q's
}

queueNode *getNextToRun(){
  queueNode *nextThread = threadQ->head;

  //TODO: implement other types of returning next from Q's and multi level Q's
  return nextThread;
}
