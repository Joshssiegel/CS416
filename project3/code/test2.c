  #include "../my_vm.h"
  #include <pthread.h>

  int countthreads = 0;

  void looptest2(void* z){
    int i = 0;
    int c_num = countthreads++;
    printf("in looptest\n");
    unsigned long p = *((unsigned long*)z);
    for(i=0;i<1024;i++){
      void *x = a_malloc(100);
      unsigned long q = 0;
      put_value(x,&p,4);
      get_value(x,&q,4);
      printf("\n(%d)q is ==> %d\nn", c_num, q);
      p++;
    }
    printf("Done thread %d\n",c_num);
  }
  int main() {


    pthread_t *thread_ids;
    printf("Before Thread\n");
    int pqr = 0;
    if(pthread_create(&thread_ids[0], NULL, looptest2, (void*)&pqr)!=0){
      printf("error creating thread\n");
    }
    // if(pthread_create(&thread_ids[1], NULL, looptest, (void*)&pqr)!=0){
    //   printf("error creating thread\n");
    // }
    int loop, numthreads = 1;
    for(loop=0;loop<numthreads;loop++){
      // printf("Joining on thread (1)\n");
      pthread_join(thread_ids[loop], NULL);
      printf("Joined on thread (%d)\n", loop);
    }
    printf("Threads have joined\n");
    return 0;
  }
