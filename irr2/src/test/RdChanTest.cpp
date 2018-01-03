#include "RenderChannel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

/*
 * [caller]    --> 
 *                 [handler]
 * [receiver]  <-- 
 */

static const unsigned loop = 100000;

unsigned genRandom(unsigned range) {
  struct timeval time;
  gettimeofday(&time, NULL);
  srand(time.tv_usec);
  return rand() % range + 1;
}

void *caller_handler(void *ch) {
  irr::RenderChannel *channel = (irr::RenderChannel *)ch;
  unsigned l = loop;
  while(l--) {
    int len = genRandom(1000);
    irr::InBuffer *in = channel->newInBuffer(len);
    channel->writeIn(in);
    int period = genRandom(500);
    printf("[caller] produces %d(bytes), wait %d(us)\n", in->length(), period);
    usleep(genRandom(period));
  }
}

void *handler_handler(void *ch) {
  irr::RenderChannel *channel = (irr::RenderChannel *)ch;
  unsigned l = loop;
  while(l--) {
    irr::InBuffer *in = nullptr;
    channel->ReadIn(in);
    if (!in) {
      printf("[handler] handles null\n");
      break;
    }
    int period = genRandom(500);
    printf("[handler] handles %d(bytes), wait %d(us)\n", in->length(), period);
    usleep(genRandom(period));
    channel->deleteInBuffer(in);
    
    int len = genRandom(200);
    irr::OutBuffer *out = channel->newOutBuffer(len);
    channel->writeOut(out);
    printf("[handler] returns %d(bytes)\n", len);
  }
}

void *receiver_handler(void *ch) {
  irr::RenderChannel *channel = (irr::RenderChannel *)ch;
  unsigned l = loop;
  while(l--) {
    irr::OutBuffer *out;
    channel->ReadOut(out);
    printf("[receiver] receives %d(bytes)\n", out->length());
    channel->deleteOutBuffer(out);
  }
}

int main(int argc, char* argv[]) {

  irr::RenderChannel channel;
  pthread_t caller;
  if (pthread_create(&caller, NULL, caller_handler, &channel)) {
    printf("Error creating caller\n");
    return 1;
  }
  
  pthread_t handler;
  if (pthread_create(&handler, NULL, handler_handler, &channel)) {
    printf("Error creating handler\n");
    return 1;
  }

  pthread_t receiver;
  if (pthread_create(&receiver, NULL, receiver_handler, &channel)) {
    printf("Error creating handler\n");
    return 1;
  }

  if (pthread_join(caller, NULL)) {
    printf("Error join caller\n");
    return 1;
  }

  if (pthread_join(handler, NULL)) {
    printf("Error join handler\n");
    return 1;
  }

  if (pthread_join(receiver, NULL)) {
    printf("Error join receiver\n");
    return 1;
  }
  
  return 1;
}
