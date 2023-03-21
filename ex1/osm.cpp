#include "osm.h"
#include <sys/time.h>
#include <iostream>

#define ROLLING_FACTOR 10
#define SEC_TO_MICRO 1000000
#define MICRO_TO_NANO 1000

void empty_func() {

}

double osm_operation_time(unsigned int iterations) {
  if (iterations == 0) {
      return -1;
    }

  struct timeval iteration_start_time{};
  struct timeval iteration_end_time{};

  double seconds = 0;
  double microseconds = 0;

  // Round iterations to be a multiplication of the ROLLING FACTOR:
  iterations = iterations + (ROLLING_FACTOR - (iterations % ROLLING_FACTOR));

  if (gettimeofday (&iteration_start_time, NULL) == -1) {
      return -1;
    }

  for (unsigned int i = 0; i < iterations; i++) {
      1+1;
      1+1;
      1+1;
      1+1;
      1+1;
      1+1;
      1+1;
      1+1;
      1+1;
      1+1;
    }

  if (gettimeofday (&iteration_end_time, NULL) == -1) {
      return -1;
    }

  seconds += (iteration_end_time.tv_sec - iteration_start_time.tv_sec);
  microseconds += (iteration_end_time.tv_usec - iteration_start_time.tv_usec);

  return (((seconds * SEC_TO_MICRO) + microseconds) * MICRO_TO_NANO) / (iterations * ROLLING_FACTOR);
}

double osm_function_time(unsigned int iterations) {
  if (iterations == 0) {
      return -1;
    }

  struct timeval iteration_start_time{};
  struct timeval iteration_end_time{};

  double seconds = 0;
  double microseconds = 0;

  // Round iterations to be a multiplication of the ROLLING FACTOR:
  iterations = iterations + (ROLLING_FACTOR - (iterations % ROLLING_FACTOR));

  if (gettimeofday (&iteration_start_time, NULL) == -1) {
      return -1;
    }

  for (unsigned int i = 0; i < iterations; i++) {
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
      empty_func();
    }

  if (gettimeofday (&iteration_end_time, NULL) == -1) {
      return -1;
    }

  seconds += (iteration_end_time.tv_sec - iteration_start_time.tv_sec);
  microseconds += (iteration_end_time.tv_usec - iteration_start_time.tv_usec);

  return (((seconds * SEC_TO_MICRO) + microseconds) * MICRO_TO_NANO) / (iterations * ROLLING_FACTOR);
}

double osm_syscall_time(unsigned int iterations) {
  if (iterations == 0) {
      return -1;
    }

  struct timeval iteration_start_time{};
  struct timeval iteration_end_time{};

  double seconds = 0;
  double microseconds = 0;

  // Round iterations to be a multiplication of the ROLLING FACTOR:
  iterations = iterations + (ROLLING_FACTOR - (iterations % ROLLING_FACTOR));

  if (gettimeofday (&iteration_start_time, NULL) == -1) {
      return -1;
    }

  for (unsigned int i = 0; i < iterations; i++) {
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
      OSM_NULLSYSCALL;
    }

  if (gettimeofday (&iteration_end_time, NULL) == -1) {
      return -1;
    }

  seconds += (iteration_end_time.tv_sec - iteration_start_time.tv_sec);
  microseconds += (iteration_end_time.tv_usec - iteration_start_time.tv_usec);

  return (((seconds * SEC_TO_MICRO) + microseconds) * MICRO_TO_NANO) / (iterations * ROLLING_FACTOR);
}