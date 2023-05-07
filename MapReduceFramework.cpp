#include "MapReduceFramework.h"
#include <pthread.h>

#define FAILURE 1

#define ERROR_MSG "system error: "
#define PTHREAD_CREATE_ERROR_MSG "failed to create thread"
#define PTHREAD_JOIN_ERROR_MSG "failed to join thread"

class JobContext {
public:
//    MapReduceClient* client;
    int threads_amount = 0;
    atomic<uint32_t> atomic_counter;
    JobState current_state;
    vector<pthread_t*> thread_ptrs;
    pthread_mutex_t joinMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t checkJoinMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t UpdateCurrentMutex = PTHREAD_MUTEX_INITIALIZER;
    bool didJoinMutex = false;
};
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

void* single_thread(void* arg) {
    atomic<uint32_t>* counter = static_cast<atomic<uint32_t>*>(arg);
    pthread_mutex_lock(&mymutex);
    cout<<"hey"<<endl;
    (*counter)++;
    pthread_mutex_unlock(&mymutex);
    return NULL;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
                            const InputVec& inputVec,
                            OutputVec& outputVec,
                            int multiThreadLevel) {
    auto job = new JobContext();
    job->atomic_counter = 0;
    job->threads_amount = multiThreadLevel;
    job->current_state.stage = UNDEFINED_STAGE;
    job->current_state.percentage = 0;

    pthread_t threads[3];
    for (int i = 0; i < multiThreadLevel; i++) {
        auto new_thread = new pthread_t();
        int a = 1;
        job->thread_ptrs.push_back(new_thread);
        if (pthread_create(new_thread,
                       NULL,
                       single_thread,
                       (void*) &(job->atomic_counter)) != 0) {
            cout << ERROR_MSG << PTHREAD_CREATE_ERROR_MSG << endl;
            exit(FAILURE);
        }
    }

    return static_cast<JobHandle>(job);
}





/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * HINT - should use the c function pthread_join.
 * @param job the JobHandle we wait till is finished
 */
void waitForJob(JobHandle job){

    auto *job_context = static_cast<JobContext*> (job);
    //check if it's the first time we call to this function with this jobhandle, if not return from function
    pthread_mutex_lock(&(job_context->checkJoinMutex));
    //todo: check if there another way to implement this
    //todo: check if I need to check if the current state is Reduce and the percantage is 100
    if(job_context->didJoinMutex) {
        pthread_mutex_unlock(&(job_context->checkJoinMutex));
        return;
    }
    pthread_mutex_unlock(&(job_context->checkJoinMutex));
    pthread_mutex_lock(&(job_context->joinMutex));
    job_context->didJoinMutex = true;
    pthread_mutex_unlock(&(job_context->joinMutex));

    //do pthread_join to all thread in job_handle
    for(auto thread:job_context->thread_ptrs ){
        if(pthread_join(*thread ,NULL)!=0){
            cout << ERROR_MSG << PTHREAD_JOIN_ERROR_MSG << endl;
            exit(FAILURE);
        }
    }
}


/**
 * this function gets a JobHandle and updates the state of the job into the given JobState struct.
 * @param job Job to update
 * @param state New state
 */
void getJobState(JobHandle job, JobState* state){
    auto *job_context = static_cast<JobContext*> (job);
    pthread_mutex_lock(&(job_context->UpdateCurrentMutex));
    job_context ->current_state = *state;
    pthread_mutex_unlock(&(job_context->UpdateCurrentMutex));
}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished.
 * After this function is called the job handle will be invalid.
 * @param job - JOb to release
 */
void closeJobHandle(JobHandle job){

}



class CounterClient : public MapReduceClient {
public:
    void map(const K1* key, const V1* value, void* context) const {

    }

    virtual void reduce(const IntermediateVec* pairs,
                        void* context) const {

    }
};

int main(int argc, char** argv) {
    CounterClient client;
    InputVec inputVec;
    OutputVec outputVec;

    auto job = startMapReduceJob(client, inputVec, outputVec, 3);
    waitForJob(job);
    waitForJob(job);
}