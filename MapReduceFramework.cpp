#include "MapReduceFramework.h"
#include <pthread.h>

#define FAILURE 1

#define ERROR_MSG "system error: "
#define PTHREAD_CREATE_ERROR_MSG "failed to create thread"
#define PTHREAD_JOIN_ERROR_MSG "failed to join thread"
#define MUTEX_ERROR_MSG "mutex call failed"
#define SEMAPHORE_CREATE_ERROR "failed to create semaphore"
#define ERR_SEMAPHORE_WAIT "failed during semaphore wait"
#define ERR_SEMAPHORE_POST "failed during semaphore post"


/**
 * How atomic counter works - it's a 64 bit number that will be reset at the start of each stage (Map, Shuffle, Reduce).
 * For each stage it will hold 3 counters:
 * 1. First 31 bit - the key-value pairs that were already processed
 * 2. Next 31 bit - total key-value pairs that should be processed
 * 3. Last 2 bit - current stage
 *
 * How to update each counter:
 * 1. Increment counter by 1
 * 2. Increment counter by (static_cast<uint64_t>(1) << 31)
 * 3. Increment counter by (static_cast<uint64_t>(1) << 62)
 *
 * How to access each counter:
 * 1. static_cast<uint32_t>(atomic_counter.load() & 0x7FFFFFFF
 * 2. static_cast<uint32_t>((atomic_counter.load() & 0x3FFFFFFF80000000) >> 31)
 * 3. static_cast<uint8_t>((atomic_counter.load() & 0xC000000000000000) >> 62)
 */


using namespace std;

enum MutexState {Unlock, Lock};

class ThreadContext;

class JobContext {
public:
    const MapReduceClient* client;
    const InputVec* inputVec;
    OutputVec* outputVec;
    int threads_amount = 0;
    atomic<uint64_t> atomic_counter;

    JobState current_state;
    vector<pthread_t*> thread_ptrs;
    pthread_mutex_t joinMutex;
    pthread_mutex_t checkJoinMutex;
    pthread_mutex_t mapMutex;
    pthread_mutex_t reduceMutex;
    pthread_mutex_t outputVectorMutex;
    pthread_mutex_t updateCurrentMutex;
    bool didJoinMutex = false;
    Barrier *barrier;
    sem_t semaphore{};
    vector<ThreadContext> threadContextVec;
    vector<IntermediateVec> shuffleOutput;
    int total_pairs_amount;

    JobContext(const MapReduceClient &client,
               const InputVec &inputVec,
               OutputVec &outputVec,
               int multiThreadLevel) :
            client(&client), inputVec(&inputVec), outputVec(&outputVec), threads_amount(multiThreadLevel),
            current_state(), thread_ptrs(), atomic_counter(0), joinMutex(PTHREAD_MUTEX_INITIALIZER),
            checkJoinMutex(PTHREAD_MUTEX_INITIALIZER), mapMutex(PTHREAD_MUTEX_INITIALIZER),
            outputVectorMutex(PTHREAD_MUTEX_INITIALIZER), reduceMutex(PTHREAD_MUTEX_INITIALIZER),
            updateCurrentMutex(PTHREAD_MUTEX_INITIALIZER), didJoinMutex(false), barrier()
    {
        if (sem_init(&semaphore, 0, 1) != 0) {
            cout << ERROR_MSG << SEMAPHORE_CREATE_ERROR << endl;
            exit(EXIT_FAILURE);
        }
    }

};

class ThreadContext{
public:
    JobContext *jobContextPtr;
    IntermediateVec* intermediateVec= new IntermediateVec();
    explicit ThreadContext(JobContext * _jobContextPtr){
        jobContextPtr = _jobContextPtr;
    }
};


pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

void* single_thread(void* arg) {
    atomic<uint64_t>* counter = static_cast<atomic<uint64_t>*>(arg);
    pthread_mutex_lock(&mymutex);
    cout<<"hey"<<endl;
    (*counter)++;
    pthread_mutex_unlock(&mymutex);
    return NULL;
}

void mutex_handler(MutexState state, pthread_mutex_t *mutex) {
    switch (state) {
        case Unlock:
            if (pthread_mutex_unlock(mutex)) {
                cout << ERROR_MSG << MUTEX_ERROR_MSG << endl;
                exit(FAILURE);
            }
            break;
        case Lock:
            if (pthread_mutex_lock(mutex)) {
                cout << ERROR_MSG << MUTEX_ERROR_MSG << endl;
                exit(FAILURE);
            }
            break;
    }
}

/**
 * The function is called from the client's map function.
 *  The function saves the intermediary element in the context data structures.
 *  In addition, the function updates the number of intermediary elements, by updating an atomic counter.
 * @param (key ,value)  intermediary element.
 * @param context - passed from the framework to the client's map function as parameter.
 *                  contains a data structure of the thread that created the intermediary element.
 */
void emit2 (K2* key, V2* value, void* context){
    auto threadContext = (ThreadContext *) context;
    auto intermediatePair = IntermediatePair(key,value);
    threadContext->intermediateVec->push_back(intermediatePair);
}

void emit3 (K3* key, V3* value, void* context) {
    auto threadContext = (ThreadContext *) context;
    auto outputPair = OutputPair(key, value);
    pthread_mutex_lock(&(threadContext->jobContextPtr->outputVectorMutex));
    threadContext->jobContextPtr->outputVec->push_back(outputPair);
    pthread_mutex_unlock(&(threadContext->jobContextPtr->outputVectorMutex));
}



/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * HINT - should use the c function pthread_join.
 * @param job the JobHandle we wait till is finished
 */
void waitForJob(JobHandle job) {
    auto *job_context = static_cast<JobContext*> (job);
    //check if it's the first time we call to this function with this jobhandle, if not return from function
    pthread_mutex_lock(&(job_context->checkJoinMutex));
    //todo: check if there another way to implement this
    //todo: check if I need to check if the current state is Reduce and the percentage is 100
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
    pthread_mutex_lock(&(job_context->updateCurrentMutex));
    switch (state->stage){
        case UNDEFINED_STAGE:
            state->percentage = 0;
            break;
        case MAP_STAGE:
            state->percentage = static_cast<uint32_t>(job_context->atomic_counter.load() & 0x7FFFFFFF) /
                    static_cast<uint32_t>((job_context->atomic_counter.load() & 0x3FFFFFFF80000000) >> 31) * 100;
            break;
        case SHUFFLE_STAGE:
        case REDUCE_STAGE:
            break;
    }
//    job_context ->current_state = *state;
    pthread_mutex_unlock(&(job_context->updateCurrentMutex));
}

void thread_map(ThreadContext* threadContext){
    auto jobContext = threadContext->jobContextPtr;
    auto inputVector = jobContext->inputVec;

    mutex_handler(Lock, &(jobContext->mapMutex));
    if (jobContext->current_state.stage == UNDEFINED_STAGE) {
        jobContext->atomic_counter.fetch_add(static_cast<uint64_t>(inputVector->size()) << 31);
        jobContext->atomic_counter.fetch_add(static_cast<uint64_t>(1) << 62);
        jobContext->current_state.stage = MAP_STAGE;
    }
    mutex_handler(Unlock, &(jobContext->mapMutex));


    auto oldValue = jobContext->atomic_counter++;
    while (oldValue <= inputVector->size()) {
        pthread_mutex_lock(&(jobContext->mapMutex));
        auto currentKey = inputVector->at(oldValue).first;
        auto currentVal = inputVector->at(oldValue).second;
        jobContext->client->map(currentKey, currentVal, jobContext);
        oldValue = jobContext->atomic_counter++;
        pthread_mutex_unlock(&(jobContext->mapMutex));
    }
}

void thread_reduce(ThreadContext* threadContext) {
    auto jobContext = threadContext->jobContextPtr;

    jobContext->atomic_counter = 0;
    mutex_handler(Lock, &(jobContext->outputVectorMutex));
    if (jobContext->current_state.stage == SHUFFLE_STAGE) {
        jobContext->atomic_counter.fetch_add(static_cast<uint64_t>(jobContext->shuffleOutput.size()) << 31);
        jobContext->atomic_counter.fetch_add(static_cast<uint64_t>(3) << 62);
        jobContext->current_state.stage = REDUCE_STAGE;
    }
    mutex_handler(Unlock, &(jobContext->outputVectorMutex));

    auto old_value = jobContext->atomic_counter++;

    mutex_handler(Lock, &(jobContext->outputVectorMutex));

    while (!jobContext->shuffleOutput.empty()) {
        mutex_handler(Lock, &(jobContext->outputVectorMutex));
        auto current_pair = jobContext->shuffleOutput.back();
        jobContext->client->reduce(&(current_pair), threadContext->jobContextPtr);
        jobContext->shuffleOutput.pop_back();
        jobContext->atomic_counter++;
        mutex_handler(Unlock, &(jobContext->outputVectorMutex));
    }

    mutex_handler(Unlock, &(jobContext->outputVectorMutex));
}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished.
 * After this function is called the job handle will be invalid.
 * @param job - JOb to release
 */
void closeJobHandle(JobHandle job) {

}



class CounterClient : public MapReduceClient {
public:
    void map(const K1* key, const V1* value, void* context) const {

    }

    virtual void reduce(const IntermediateVec* pairs,
                        void* context) const {

    }
};

void* single_thread_task(void* arg) {
    auto _threadContext = new ThreadContext(static_cast<JobContext*>(arg));

    //map
    thread_map(_threadContext);

    //sort
    sort(_threadContext->intermediateVec->begin(),_threadContext->intermediateVec->end(),comparator);

    //barrier
//    _threadContext->jobContextPtr->barrier->barrier(); // TODO - remove comment (now it's here because this is not compiling)

    //shuffle
    if (sem_wait(&_threadContext->jobContextPtr->semaphore) != 0) {
        cout << ERROR_MSG << ERR_SEMAPHORE_WAIT << endl;
        exit(EXIT_FAILURE);
    }
    shuffle(_threadContext); // TODO - aren't we calling it now for all the threads?
    for (int i = 0; i < _threadContext->jobContextPtr->threadContextVec.size(); ++i) {
        if (sem_post((&_threadContext->jobContextPtr->semaphore)) != 0) {
            cout << ERROR_MSG << ERR_SEMAPHORE_POST << endl;
            exit(EXIT_FAILURE);
        }
    }

    //reduce

    return nullptr; // TODO - what should be returned?
}

JobHandle startMapReduceJob(const MapReduceClient& client,
                            const InputVec& inputVec,
                            OutputVec& outputVec,
                            int multiThreadLevel) {
    auto job = new JobContext(client, inputVec, outputVec, multiThreadLevel);
    job->atomic_counter = 0;
    job->current_state.stage = UNDEFINED_STAGE;
    job->current_state.percentage = 0;

    pthread_t threads[3];
    for (int i = 0; i < multiThreadLevel; i++) {
        auto new_thread = new pthread_t();
        auto thread_context = new (nothrow) ThreadContext (job);
        job->threadContextVec.push_back(*thread_context);
        job->thread_ptrs.push_back(new_thread);
        if (pthread_create(new_thread,
                           NULL,
                           single_thread_task,
                           (void *) job) != 0) {
            cout << ERROR_MSG << PTHREAD_CREATE_ERROR_MSG << endl;
            exit(FAILURE);
        }
    }

    return static_cast<JobHandle>(job);
}

int main(int argc, char** argv) {
    CounterClient client;
    InputVec inputVec;
    OutputVec outputVec;

    auto job = startMapReduceJob(client, inputVec, outputVec, 3);
    waitForJob(job);
    waitForJob(job);
}