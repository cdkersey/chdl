#include "stopwatch.h"
#include <iostream>
#include <iomanip>

#include <stdint.h>
#include <sys/time.h>

#define STACK_SIZE 8

static unsigned usec_sp = 0;
static uint64_t usec[STACK_SIZE];

using namespace std;

void stopwatch_start() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  usec[usec_sp] = tv.tv_sec * 1000000l + tv.tv_usec;
  usec_sp++;
}

double stopwatch_stop() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  usec_sp--;

  uint64_t time = tv.tv_sec * 1000000l + tv.tv_usec - usec[usec_sp];

  return time * 1e-3;
}
