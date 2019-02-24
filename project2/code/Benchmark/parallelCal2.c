// File:	parallelCal.c
// Author:	Yujie REN
// Date:	09/23/2017
#include <time.h>//added
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "../my_pthread_t.h"

#define DEFAULT_THREAD_NUM 4

#define C_SIZE 100000
#define R_SIZE 10000

pthread_mutex_t   mutex;

int thread_num;

int* counter;
pthread_t *thread;

int*    a[R_SIZE];
int	 pSum[R_SIZE];
int  sum = 0;

/* A CPU-bound task to do parallel array addition */
int parallel_calculate(void* arg) {
	printf("Starting Parallel Calculating %d\n ", (1+*(int*)arg));//delete
	int i = 0, j = 0;
	int n = *((int*) arg);

	for (j = n; j < R_SIZE; j += thread_num) {
		for (i = 0; i < C_SIZE; ++i) {
			pSum[j] += a[j][i] * i;
			//if (i % 100000 == 0)
				//printf("test %d ...\n", n);
		}
	}
	for (j = n; j < R_SIZE; j += thread_num) {
		 pthread_mutex_lock(&mutex);
		 printf("Locked mutex from thread (%d)\n", (1+*(int*)arg));
		sum += pSum[j];
		 printf("Unlocking mutex from thread (%d)\n", (1+*(int*)arg));
		 pthread_mutex_unlock(&mutex);
	}
	printf("finished function parallel calculate %d\n",(*(int*)arg+1));
	return (1+*(int*)arg);
	// pthread_exit(NULL);


}

/* verification function */
void verify() {
	int i = 0, j = 0;
	sum = 0;
	memset(&pSum, 0, R_SIZE*sizeof(int));

	for (j = 0; j < R_SIZE; j += 1) {
		for (i = 0; i < C_SIZE; ++i) {
			pSum[j] += a[j][i] * i;
			//printf("pSum is: %d\n",pSum[j]);
		}
	}
	printf("\nverifying?\n");

	for (j = 0; j < R_SIZE; j += 1) {
		sum += pSum[j];
	}
	printf("\nverified sum is: %d\n", sum);
}

int main(int argc, char **argv) {
	int i = 0, j = 0;

	if (argc == 1) {
		thread_num = DEFAULT_THREAD_NUM;
	} else {
		if (argv[1] < 1) {
			printf("enter a valid thread number\n");
			return 0;
		} else
			thread_num = atoi(argv[1]);
	}

	// initialize counter
	counter = (int*)malloc(thread_num*sizeof(int));
	for (i = 0; i < thread_num; ++i)
		counter[i] = i;

	// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	// initialize data array
	for (i = 0; i < R_SIZE; ++i)
		a[i] = (int*)malloc(C_SIZE*sizeof(int));

	for (i = 0; i < R_SIZE; ++i)
		for (j = 0; j < C_SIZE; ++j)
			a[i][j] = j;

	memset(&pSum, 0, R_SIZE*sizeof(int));

	// mutex init
	// pthread_mutex_init(&mutex, NULL);

	struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

	for (i = 0; i < thread_num; ++i)
		pthread_create(&thread[i], NULL, &parallel_calculate, &counter[i]);
		int r=0;
		int *rval = &r;

	//my_pthread_schedule(0);
	printf("ABOUT TO JOIN\n");
	for (i = 0; i < thread_num; ++i){
		printf("JOINING ON (%d)\n", thread[i]);
		pthread_join(thread[i], rval);
		printf("&& value of rturn is: %d or %d\n",r,*rval);
		// printf("In MAIN, after waiting on thread (%d), we got retval as ==> (%d)\n", i+1, rval);
	}

	clock_gettime(CLOCK_REALTIME, &end);
    printf("running time: %lu milli-seconds\n", (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

	printf("sum is: %d\n", sum);

	// mutex destroy
	// pthread_mutex_destroy(&mutex);

	// feel free to verify your answer here:
	verify();

	// Free memory on Heap
	// printf("\nfreeing thread array\n");
	//  free(thread);
	//  printf("\nfreeing counter\n");
	//  free(counter);
	//  printf("\nfreeing a[i]\n");
	// for (i = 0; i < R_SIZE; ++i)
	// 	free(a[i]);
  printf("********Done everything!!!!!!!!!**********\n");
	return 0;
}