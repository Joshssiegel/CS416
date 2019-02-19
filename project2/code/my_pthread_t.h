// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	January 2019

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* To use real pthread Library in Benchmark, you have to comment the USE_MY_PTHREAD macro */
#define USE_MY_PTHREAD 1

#define STACK_SIZE 1048576//A megabyte
#define TIME_QUANTUM 10//milliseconds


/* include lib header files that you need here: */
#include <time.h>//added
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <malloc.h>

 typedef enum _status{
   READY,RUNNING,DONE
 }status;
typedef uint my_pthread_t; // a integer identifier
typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
  my_pthread_t *threadId;
	// thread status
  status thread_status;
	// thread context
  ucontext_t context;
  ucontext_t return_context;

	// thread stack
  //We think this is part of the context
	// thread priority
  int priority; //0 is highest priority

	// And more ...

	// YOUR CODE HERE
  int time_quantum_counter;
} tcb;

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */

	// YOUR CODE HERE
} my_pthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)
typedef struct _queueNode{
  tcb* thread_tcb;
  struct _queueNode* next;
} queueNode;

typedef struct _threadQueue {
  struct _queueNode* head;
  struct _queueNode* tail;
} threadQueue;
// YOUR CODE HERE
/* Function Declarations: */

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

/*handle SIGALRM*/
void SIGALRM_Handler();

/*When a thread exits, mark it as done*/
void processFinishedJob(int threadID);

/*Search for a thread by its threadID*/
tcb* findThread(int threadID);

static void schedule();

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif

#endif
