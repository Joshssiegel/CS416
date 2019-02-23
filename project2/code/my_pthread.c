// File:	my_pthread.c
// Author:	Yujie REN
// Date:	January 2019

// List all group member's name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"
#include <execinfo.h>

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
//ucontext_t scheduleContext=NULL;
ucontext_t schedulerContext;
ucontext_t parentContext;
int threadCounter=0;
int mutexCounter=0;
threadQueue* threadQ=NULL;
mutexNode* mutexList=NULL;
my_pthread_mutex_t qLock;
int ignoreSignal=0;
//ucontext_t processFinishedJobContext;

void threadWrapper(void * arg, void *(*function)(void*), int threadId){
  // Function call: makecontext(&newThreadContext, (void*)threadWrapper, 2, arg, (void*)function);
  void * threadReturnValue = (*function)(arg);//
  printf("DONE THREAD ==> (%d), in wrapper with return value ==> (%d)\n", threadReturnValue, (threadId));
  returnValues[threadId] = threadReturnValue; // saving the threads return value in an array
  // void * returnValue = function((int)arg);//
}

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
  //BLOCK TIMER FROM INTERRPTING
  ignoreSignal=1;
 /*sigset_t signal_set;
 sigemptyset(&signal_set);
 sigaddset(&signal_set, SIGALRM);
 sigprocmask(SIG_BLOCK, &signal_set, NULL);*/
  *thread=++threadCounter;
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



  printf("thread ID is? %d",*thread);
	// Create Thread Control Block
  tcb* new_tcb=(tcb*) malloc(sizeof(tcb));
  new_tcb->priority=0;
  //new_tcb->time_quantum_counter=0;
  new_tcb->time_ran=0;

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
  // makecontext(&newThreadContext, (void*)function, 1, arg);
  makecontext(&newThreadContext, (void*)threadWrapper, 3, arg, function, (int)new_tcb->threadId);
  new_tcb->context=newThreadContext;

  queueNode* qNode =(queueNode*) malloc(sizeof(queueNode*));
  qNode->thread_tcb=new_tcb;
  qNode->next=NULL;




  if(threadQ==NULL)
  {
    signal(SIGALRM, SIGALRM_Handler);//SIGALRM_Handler will call scheduler
    start_timer(TIME_QUANTUM);//setting timer to fire every TIME_QUANTUM milliseconds
    if(SCHED == MLFQ_SCHEDULER){
      ;
    }
    else if(SCHED == STCF_SCHEDULER){
      ;
    }
    else if(SCHED == FIFO_SCHEDULER){
      threadQ=(threadQueue*) malloc(sizeof(threadQueue));
      //initialize the head and tail
      threadQ->head=qNode;
      threadQ->tail=qNode;
    }
    printf("Made the queue\n");
    pthread_mutex_init(&qLock,NULL);
    printf("getting main context\n");
    getcontext(&parentContext);

  }
  else{
    if(SCHED == MLFQ_SCHEDULER){
      ;
    }
    else if(SCHED == STCF_SCHEDULER){
      ;
    }
    else if(SCHED == FIFO_SCHEDULER){
      threadQ->tail->next=qNode;
      threadQ->tail=qNode;
    }
  }
  //ALLOW TIMER TO CONTINUE
  //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);

  ignoreSignal=0;
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

  // TODO: save the return value



  ignoreSignal=1;
  queueNode* finishedQNode = getTopOfQueue();
  tcb* finishedThread=finishedQNode->thread_tcb;
  //Set to done
  printf("\nA job just decided to exit with ID %d \n",finishedThread->threadId);
  finishedThread->thread_status=DONE;
  ignoreSignal=0;
  SIGALRM_Handler();

}


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
  ignoreSignal=1;
  printf("\n\n\n\n\n******************JOIN (%d)**************\n", thread);
	// Waiting for a specific thread to terminate
	// Once this thread finishes,
	// Deallocated any dynamic memory created when starting this thread
	// YOUR CODE HERE
  //TODO: returning Value_ptr
  //

  //First, check if thread is in the queueNode
  tcb* thread_to_join=findThread(thread);

  //If thread is in queue, check its status
    //If thread status is Done, free it, and swap context with Parent
    //If thread status is READY or RUNNING, yield CPU
  //If thread is not in Q, set context to parent
  if(thread_to_join==NULL){
    printf("Thread to join (%d) on is no longer in queue\n",thread);
    if(value_ptr!=NULL){
      *value_ptr = returnValues[(int)thread];
      printf("In JOIN, thread ==> (%d) has return value ==> (%d)\n", thread, returnValues[thread]);
    }

    //TODO: Return value_ptr if not NULL
    ignoreSignal=0;
    return 0;
  }

  //Thread is in Queue
  else{
    //If thread is Not done, yield CPU
    if(thread_to_join->thread_status!=DONE){
      printf("thread to join (%d) is still executing, yielding CPU\n",thread);
      thread_to_join->join_boolean = 1;
      //schedule();
      ignoreSignal=0;
      SIGALRM_Handler();
      // Thread being JOINED on is now DONE
      if(value_ptr!=NULL){
        *value_ptr = returnValues[(int)thread_to_join->threadId];
        printf("In JOIN, thread ==> (%d) has return value ==> (%d)\n", (int)thread_to_join->threadId, returnValues[(int)thread_to_join->threadId]);
      }
      printf("Done yielding the CPU after JOIN on thread (%d)\n", thread_to_join->threadId);
      return 0;
      //my_pthread_yield();
      //WHAT DO WE DO HERE???
    }
    //If thread is done, free and return? or setContext to Main?
    else{
      printf("thread to join (%d) is done\n",thread);
      if(value_ptr!=NULL){
        *value_ptr = returnValues[(int)thread_to_join->threadId];
        printf("In JOIN, thread ==> (%d) has return value ==> (%d)\n", (int)thread_to_join->threadId, returnValues[(int)thread_to_join->threadId]);
      }
      free(thread_to_join->context.uc_stack.ss_sp);
      free(thread_to_join->return_context.uc_stack.ss_sp);
      free(thread_to_join);
      //TODO: Remove thread from Queue
      //TODO: Return value_ptr if not nULL
      ignoreSignal=0;
      return 0;
  }
  printf("Shouldn't reach here (end of join)\n");
  ignoreSignal=0;
	return 0;
}
}

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
  // Initialize data structures for this mutex
  mutexNode* newMutexNode=(mutexNode*) malloc(sizeof(mutexNode));
  //printf("mutex before Malloc is: %d\n",mutex);
  my_pthread_mutex_t* new_mutex=(my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));
  newMutexNode->mutex=new_mutex;
  newMutexNode->mutex->isLocked=0;
  newMutexNode->mutex->mutexId=++mutexCounter;
  newMutexNode->next=mutexList;
  mutexList=newMutexNode;
  *mutex=*new_mutex;

  printf("mutex after Malloc is: %d\n",mutex->mutexId);

  printf("made mutex %d\n",mutexCounter);
	// YOUR CODE HERE
	return 0;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// Use the built-in test-and-set atomic function to test the mutex
	// If mutex is acquired successfuly, enter critical section
	// If acquiring mutex fails, push current thread into block list
	// and context switch to scheduler
  //printf("locking mutex: %d\n",mutex->mutexId);
  mutexNode *mutexToLock = findMutex(mutex->mutexId);

  if(mutexToLock == NULL){
    printf("Mutex %d has not been initialized\n",mutex->mutexId);
    return -1;
  }
  else{
    //check that it hasn't already been locked
    //while(mutexToLock->mutex->isLocked==1){// we want to yield to other threads, until critical section is unlocked


    while(__sync_lock_test_and_set(&(mutexToLock->mutex->isLocked),1)==1){// we want to yield to other threads, until critical section is unlocked
        //printf("mutex %d is locked, yielding cpu\n",mutexToLock->mutex->mutexId);
        my_pthread_yield();
    }
    mutexToLock->mutex->isLocked = 1;
    //printf("Successfully locked mutex %d\n",mutexToLock->mutex->mutexId);
    //printf("We locked the mutex, see: %d",mutexToLock->mutex->isLocked);
    return 0;
  }
	// YOUR CODE HERE
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.
  mutexNode *mutexToUnlock = findMutex(mutex->mutexId);
  if(mutexToUnlock==NULL)
  {
    printf("No mutex to unlock\n");
    return -1;
  }
  if(__sync_lock_test_and_set(&(mutexToUnlock->mutex->isLocked),0)==0){
    printf("Mutex is already unlocked!\n");
    return -1;
  }
  //else we are already unlocked it
  return 0;
}


/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in my_pthread_mutex_init
  printf("yo I am gonna like totally destroy you (a mutex) now\n");
  mutexNode *mutexToDestroy = findMutex(mutex->mutexId);
  if(mutexToDestroy==NULL || mutexToDestroy->mutex->isLocked==1){
    printf("Cannot destroy right now\n");
    return -1;
  }
  mutexNode *mutexPtr = mutexList;
  mutexNode *mutexPrev = mutexPtr;
  while(mutexPtr!=NULL){
    if(mutexPtr->mutex->mutexId==mutexToDestroy->mutex->mutexId){// found
      printf("Mutex found!!!\n");
      mutexPrev->next = mutexPtr->next;
      free(mutexPtr);
      return 0;
    }
    mutexPrev = mutexPtr;
    mutexPtr = mutexPtr->next;
  }
  // mutex not found
  // shouldn't get here
	return -1;
}

struct timespec timeCheck;
int firstSchedule=1;


void SIGALRM_Handler(){
  start_timer(TIME_QUANTUM);//setting timer to fire every TIME_QUANTUM milliseconds
  if(ignoreSignal==1){
    printf("\n\n******************Interrupt Signal was ignored*****************************\n");
    return;
  }
  ignoreSignal=1;
  //TODO: Check if parentContext is the one who was interrupted. Could be thread or Main
  //swapcontext(&parentContext,&schedulerContext);

  //schedule();
  //printf("Ok resumin now\n");
  //get the thread that was just running.
  printf("Interrupted!\n");
  queueNode* finishedThread=getTopOfQueue();

    //No jobs in queue
  if(finishedThread==NULL)
  {
    printf("No jobs in queue\n");
    printf("\nInterrupted from Main, no jobs left in queue, going back to main\n");

    ignoreSignal=0;
    return;
    //setcontext(&parentContext);
  }
  //Check if top of queue is ready
  else if(finishedThread->thread_tcb->thread_status==READY){
    printf("\nInterrupted from Main we think, switching to schedule context\n");
    int swapStatus=swapcontext(&parentContext,&schedulerContext);
    if(swapStatus!=0){
      printf("Error swapping main and scheduler: %d\n",swapStatus);
    }
     printf("\nResuming Main\n");

     return;
  }
  //top of queue was Running or Done
  else{
    printf("\ninterrupted from thread %d\n",(finishedThread->thread_tcb->threadId));
    int swapStatus=swapcontext(&(finishedThread->thread_tcb->context),&schedulerContext);
    if(swapStatus!=0){
      printf("Error swapping top of queue and scheduler: %d\n",swapStatus);
    }
    printf("resuming thread %d\n",(finishedThread->thread_tcb->threadId));
    return;
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

/* FIFO scheduling algorithm */
static void sched_fifo() {
	// Your own implementation of FIFO
  /*sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGALRM);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);*/


  struct timespec prev_time=timeCheck;
  clock_gettime(CLOCK_REALTIME, &timeCheck);
  unsigned long timeRan=0;
  if(!firstSchedule){
    timeRan=(timeCheck.tv_sec - prev_time.tv_sec) * 1000 + (timeCheck.tv_nsec - prev_time.tv_nsec) / 1000000;
    printf("scheduling time: %lu milli-seconds\n", timeRan);
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
    //ALLOW TIMER TO CONTINUE
    //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
    ignoreSignal=0;
    int setStatus=setcontext(&parentContext);
    if(setStatus!=0){
      printf("\nSwap error but shouldnt be here anyway, error is: %d \nI'm exiting now\n",setStatus);
      exit(0);
    }
  }
  //Check if top of queue is ready
  if(finishedThread->thread_tcb->thread_status==READY){
    printf("Top of queue was not running (ready).\n");
    finishedThread->thread_tcb->thread_status=RUNNING;
    //Finished thread never actuall ran, so run it.
    // int setStatus=setcontext(&(finishedThread->thread_tcb->context));

    //TODO: Make sure main was saved in sigalarm
    //ALLOW TIMER TO CONTINUE
    //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
    ignoreSignal=0;
    int setStatus=setcontext(&(finishedThread->thread_tcb->context));
    if(setStatus!=0){
      printf("\nOOPSIES, Swap no work in starting top of queue, error is: %d \nI'm exiting now\n",setStatus);
      exit(0);
    }
    printf("should never reach here 1.\n");
    //exit(0);
  }
  //Top of queue has finished executing
  else if(finishedThread->thread_tcb->thread_status==DONE)
  {
    printf("thread (%d) is done, removing\n", finishedThread->thread_tcb->threadId);

    // threadQ->head=finishedThread->next; // removing
    int join_boolean=finishedThread->thread_tcb->join_boolean;
    int finishedThreadId=finishedThread->thread_tcb->threadId;
    removeFromQueue(finishedThread);
    // queueNode* threadToRun=threadQ->head; // get next to run
    queueNode* threadToRun=getTopOfQueue(); // get next to run

    if(join_boolean==1){
      //swap to main
      printf("THREAD (%d) to join is DONE, returning to main\n", finishedThreadId);
      // free(finishedThread); ADD THIS
      //ALLOW TIMER TO CONTINUE
      //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
      ignoreSignal=0;
      int setStatus=setcontext(&parentContext);
      printf("Just set context to join (main) shouldn't be here, set Status is: %d\n",setStatus );
      if(setStatus!=0){
        printf("\nOOPSIES, Swap no work before returning to join, error is: %d \nI'm exiting now\n",setStatus);
        exit(0);
      }
    }
    if(threadToRun==NULL)
    {
      printf("No more threads to run after removing last thread switching back to main\n");
      //ALLOW TIMER TO CONTINUE
      //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
      ignoreSignal=0;
      int setStatus=setcontext(&parentContext); // done executing everything in Q, switching back to main
      printf("if this prints set failed to go back to main\n");

      if(setStatus!=0){
        printf("\nOOPSIES, Swap no work when no threads to run, error is: %d \nI'm exiting now\n",setStatus);
        exit(0);
      }
      //continue;
    }
    else{
      printf("\nthread (%d) is going to run next\n", threadToRun->thread_tcb->threadId);
      threadToRun->thread_tcb->thread_status=RUNNING;
      //Swap context
      //Set context starts from the top
      //ALLOW TIMER TO CONTINUE
      //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
      ignoreSignal=0;
      int setStatus=setcontext(&(threadToRun->thread_tcb->context));
      //int setStatus=swapcontext(&parentContext,&(threadToRun->thread_tcb->context));
      if(setStatus!=0){
        printf("OOPSIES, Set no work after removing a done thread and starting another one, error is: %d \nI'm exiting now\n",setStatus);
        exit(0);

      }
    }

  }
  else{
  //Swapping two running threads

  //Change the status of the finished thread to ready
  finishedThread->thread_tcb->thread_status=READY;
  finishedThread->thread_tcb->time_ran+=timeRan;
  printf("about to swap out running Thread: %d which has ran for %ul milliseconds\n",(finishedThread->thread_tcb->threadId),finishedThread->thread_tcb->time_ran);
  //Move it to the back
  threadQ->tail->next=finishedThread;
  threadQ->tail=finishedThread;
  threadQ->head=threadQ->head->next;
  finishedThread->next=NULL;
  //Setup next thread to run
  // queueNode* threadToRun=threadQ->head;
  queueNode* threadToRun=getTopOfQueue();
  threadToRun->thread_tcb->thread_status=RUNNING;
  //Swap context saves lace in old context for later
  //int setStatus=swapcontext(&(finishedThread->thread_tcb->context),&(threadToRun->thread_tcb->context));
  //ALLOW TIMER TO CONTINUE
  //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
  ignoreSignal=0;
  int setStatus=setcontext(&(threadToRun->thread_tcb->context));
  if(setStatus!=0){
    printf("OOPSIES, Swap no work between threads, error is: %d \nI'm exiting now\n",setStatus);
    exit(0);
  }
  printf("\nset staus should be 0 after swapping two threads:  %d\n",setStatus);
  }

  // Invoke different actual scheduling algorithms
  // according to policy (STCF or MLFQ)
  printf("end of scheduler, should not be here,\n");
  //ALLOW TIMER TO CONTINUE
  //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
  ignoreSignal=0;
  setcontext(&parentContext);
  // if (sched == STCF)
  //		sched_stcf();
  // else if (sched == MLFQ)
  // 		sched_mlfq();

  // YOUR CODE HERE
}
/* scheduler */
static void schedule() {
  //TODO: context switch to schedule function
  // Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function
  //printf("scheduling\n");
  //Prevent schedule from being interrupted


  // schedule policy
  if(SCHED == MLFQ_SCHEDULER){
  	// Choose MLFQ
    printf("MLFQ\n");
    sched_mlfq();
  }
  else if(SCHED == FIFO_SCHEDULER)
  {
  	// Choose FIFO
    printf("FIFO\n");
    sched_fifo();
  }
  else{
    //Choose STCF
    printf("STCF\n");
    sched_stcf();
  }

}

void printQ(){
  PRINTFUNC
  queueNode *ptr = threadQ->head;
  printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
  while(ptr!=NULL){
    printf("(%d)===>", ptr->thread_tcb->threadId);
    ptr=ptr->next;
  }
  printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}
// Feel free to add any other functions you need

// YOUR CODE HERE
//Marks finished Threads as DONE
void processFinishedJob(int threadID){
  /*sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGALRM);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);*/
  ignoreSignal=1;
  printf("\nA job just finished!!!! with ID %d (This is so good :) \n",threadID);
  tcb* finishedThread=findThread(threadID);
  printf("Found thread! about to interrupt to remove this thread!\n");
  finishedThread->thread_status=DONE;

  //sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
  ignoreSignal=0;
  SIGALRM_Handler();
}

/*Search for a thread by its threadID*/
tcb* findThread(int threadID){

  if(SCHED == MLFQ_SCHEDULER){
    ;
  }
  else if(SCHED == STCF_SCHEDULER){
    ;
  }
  else if(SCHED == FIFO_SCHEDULER){
    //pthread_mutex_lock(&qLock);
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
        printf("did we actually find the thread: %d??\n",head->thread_tcb->threadId);
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
    //pthread_mutex_unlock(&qLock);
  }

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
  //pthread_mutex_lock(&qLock);
  if(SCHED == MLFQ_SCHEDULER){
    ;
  }
  else if(SCHED == STCF_SCHEDULER){
    ;
  }
  else if(SCHED == FIFO_SCHEDULER){
    if(threadQ==NULL){
      printf("Queue is NULL!!!\n");
      //pthread_mutex_unlock(&qLock);

      return NULL;
    }
    queueNode* topOfQueue=threadQ->head;

    if(topOfQueue==NULL){
       printf("Head is NULL\n");
       //pthread_mutex_unlock(&qLock);

      return NULL;
    }
    else{
      //pthread_mutex_unlock(&qLock);
      return topOfQueue;
    }
  }
  // ifndef MLFQ
  // MLFQ top of queue
}

void removeFromQueue(queueNode *finishedThread){
  //Gonna have to change this logic
  // CURRENTLY this function removes whatever is the HEAD of the Q
  //pthread_mutex_lock(&qLock);
  if(SCHED == MLFQ_SCHEDULER){
    ;
  }
  else if(SCHED == STCF_SCHEDULER){
    ;
  }
  else if(SCHED == FIFO_SCHEDULER){
    if(finishedThread==NULL){
      printf("Cannot remove NULL thread\n");
      //pthread_mutex_unlock(&qLock);

      return;
    }
    if(threadQ==NULL || threadQ->head==NULL)
    {
      printf("Queue was empty, cannot remove\n");
      //pthread_mutex_unlock(&qLock);
      return;
    }
    if(threadQ->head->thread_tcb->threadId==finishedThread->thread_tcb->threadId){
      queueNode* tmp=threadQ->head;
      threadQ->head=threadQ->head->next;
      freeQueueNode(tmp);
      printf("Removed head\n");
      //pthread_mutex_unlock(&qLock);
      return;
    }
    queueNode* prev=threadQ->head;
    queueNode* current=threadQ->head->next;
    while(current!=NULL){
      if(current->thread_tcb->threadId==finishedThread->thread_tcb->threadId){
        printf("removing thread: %d\n",current->thread_tcb->threadId);
        prev->next=current->next;
        freeQueueNode(current);
        //pthread_mutex_unlock(&qLock);
        return;
      }
      prev=prev->next;
      current=current->next;
    }
    printf("Could not find thread to remove\n");
    //pthread_mutex_unlock(&qLock);
    return;
  }
  //TODO: implement other types of removing from Q's and multi level Q's
  return;
}

// queueNode *getNextToRun(){
//   if(SCHED == MLFQ_SCHEDULER){
//     ;
//   }
//   else if(SCHED == STCF_SCHEDULER){
//     ;
//   }
//   else if(SCHED == FIFO_SCHEDULER){
//     queueNode *nextThread = threadQ->head;
//   }
//
//   //TODO: implement other types of returning next from Q's and multi level Q's
//   return nextThread;
// }
mutexNode *findMutex(int mutexId){// searches and returns mutex that has a matching mutexId
  ignoreSignal=1;
  mutexNode *mutexPtr = mutexList;
  while(mutexPtr!=NULL){
    if(mutexPtr->mutex->mutexId==mutexId){
      ignoreSignal=0;
      return mutexPtr;
    }
    mutexPtr = mutexPtr->next;
  }
  ignoreSignal=0;
  return NULL;
}
void freeQueueNode(queueNode* node){
  tcb* tcbToFree=node->thread_tcb;
  freeTcb(tcbToFree);
  free(node);
  printf("Freed queue node too\n");
  return;
}
void freeTcb(tcb* tcb){
  printf("Freed allocated stack for thread %d\n",tcb->threadId);
  free(tcb->context.uc_stack.ss_sp);
  free(tcb->return_context.uc_stack.ss_sp);
  return;
}
