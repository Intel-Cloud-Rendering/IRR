#include "BufferQueue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

static const unsigned loop = 10000;

unsigned genRandom(unsigned range) {
  struct timeval time;
  gettimeofday(&time, NULL);
  srand(time.tv_usec);
  return rand() % range + 1;
}

void *producer_handler(void *pQue) {
  irr::BufferQueue<int> *pBufQue = (irr::BufferQueue<int> *)pQue;
  unsigned l = loop;
  while(l--) {
    int num = genRandom(100);
    int period = genRandom(500);
    pBufQue->push(num);
    printf("produce %d, wait %d(s)\n", num, period);
    usleep(genRandom(period));
  }
}

void *consumer_handler(void *pQue) {
  irr::BufferQueue<int> *pBufQue = (irr::BufferQueue<int> *)pQue;
  unsigned l = loop;
  while(l--) {
    int num;
    pBufQue->wait_and_pop(num);
    printf("consume %d\n", num);
  }
}

int main(int argc, char* argv[]) {
  /*
  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
  */
  
  irr::BufferQueue<int> bufQue;
  pthread_t producer;
  if (pthread_create(&producer, NULL, producer_handler, &bufQue)) {
    printf("Error creating producer\n");
    return 1;
  }
  
  pthread_t consumer;
  if (pthread_create(&consumer, NULL, consumer_handler, &bufQue)) {
    printf("Error creating consumer\n");
    return 1;
  }
    
  if (pthread_join(producer, NULL)) {
    printf("Error join producer\n");
    return 1;
  }

  if (pthread_join(consumer, NULL)) {
    printf("Error join consumer\n");
    return 1;
  }
  
  return 1;
}
