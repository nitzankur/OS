#ifndef MAPREDUCEFRAMEWORK_H
#define MAPREDUCEFRAMEWORK_H

#include "MapReduceClient.h"
#include <iostream>
using namespace std;

/**
 * job - an identifier of a running job. Returned when starting a job and used
 * by other framework functions (for example to get the state of a job).
 */

/**
 * an identifier of a running job. Returned when starting a job and used
 * by other framework functions (for example to get the state of a job).
 */
typedef void* JobHandle;

enum stage_t {UNDEFINED_STAGE=0, MAP_STAGE=1, SHUFFLE_STAGE=2, REDUCE_STAGE=3};

typedef struct {
    /**
    * We will save the job stage using that enum.
    * The job should be at an undefined stage until the first thread starts the map phase.
    */
	stage_t stage;

    /**
     * float percentage – job progress of current stage (i.e., the percentage of elements that
     * were processed out of all the elements that should be processed in the stage, number in
     * the range of 0 to 100).
     */
	float percentage;
} JobState;

void emit2 (K2* key, V2* value, void* context);
void emit3 (K3* key, V3* value, void* context);

/**
 * starts running the MapReduce algorithm (with several
 *   threads) and returns a JobHandle.
 * @param client The implementation of MapReduceClient or in other words the task that the framework should run.
 * @param inputVec a vector of type std::vector<std::pair<K1*, V1*>>, the input elements.
 * @param outputVec a vector of type std::vector<std::pair<K3*, V3*>>, to which the output
 * elements will be added before returning. You can assume that outputVec is empty.
 * @param multiThreadLevel the number of worker threads to be used for running the algorithm.
 * You will have to create threads using c function pthread_create. You can assume
 * multiThreadLevel argument is valid (greater or equal to 1).
 * @return The function returns JobHandle that will be used for monitoring the job.
 */
JobHandle startMapReduceJob(const MapReduceClient& client,
	const InputVec& inputVec,
    OutputVec& outputVec,
	int multiThreadLevel);

/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * HINT - should use the c function pthread_join.
 * @param job the JobHandle we wait till is finished
 */
void waitForJob(JobHandle job);

/**
 * this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job Job to update
 * @param state New state
 */
void getJobState(JobHandle job, JobState* state);

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished.
 * After this function is called the job handle will be invalid.
 * @param job - JOb to release
 */
void closeJobHandle(JobHandle job);
	
	
#endif //MAPREDUCEFRAMEWORK_H
