#include "MapReduceFramework.h"
#include <pthread.h>
#include <bits/stdc++.h>
#include "Barrier/Barrier.h"
#include <semaphore.h>

#define FAILURE 1

#define ERROR_MSG "system error: "
#define PTHREAD_CREATE_ERROR_MSG "failed to create thread"
#define PTHREAD_JOIN_ERROR_MSG "failed to join thread"
#define MUTEX_ERROR_MSG "mutex call failed"
#define SEMAPHORE_CREATE_ERROR "failed to create semaphore"
#define ERR_SEMAPHORE_WAIT "failed during semaphore wait"
#define ERR_SEMAPHORE_POST "failed during semaphore post"
#define MUTEX_DESTROY_ERROR "failed to destroy mutex"
#define SEMAPHORE_DESTROY_ERROR "failed to destroy semaphore"

#define ATOMIC_COUNTER_PRINT "atomic counter value = "
#define ATOMIC_PERCENTAGE_GOAL_PRINT "percentage goal value = "
#define ATOMIC_STATE_PRINT "atomic state value = "
#define OLD_VALUE_PRINT "old value = "

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

//TODO: ask in the forum about empty string and empty vectors
//TODO: ask in the forum about add pthread_flag to makefile
//TODO: ask in the forum about whether we should update the stage percentage during the run, or if it's just the getJobState responsibility



using namespace std;

enum MutexState {Unlock, Lock};

class ThreadContext;
class JobContext;


class ThreadContext{
public:
    JobContext *jobContextPtr;
    IntermediateVec* intermediateVec = new IntermediateVec();
    static int count_thread;
    Barrier* barrier;
    ThreadContext(JobContext * _jobContextPtr,Barrier* _barrier) {
        jobContextPtr = _jobContextPtr;
        barrier = _barrier;
    }
};


class JobContext {
public:
    const MapReduceClient* client;
    const InputVec* inputVec;
    OutputVec* outputVec;
    atomic<uint64_t>* atomic_counter;

    JobState current_state;
    vector<pthread_t*> thread_ptrs;
    pthread_mutex_t checkJoinMutex;
    pthread_mutex_t mapMutex;
    pthread_mutex_t reduceMutex;
    pthread_mutex_t outputVectorMutex;
    pthread_mutex_t updateCurrentMutex;
    bool didJoinMutex = false;
    bool callShuffle= false;
    Barrier* barrier;
    sem_t semaphore{};
    vector<ThreadContext*> threadContextVec;
    vector<IntermediateVec> shuffleOutput;
    unsigned long int total_pairs_amount;

    JobContext(const MapReduceClient &client,
               const InputVec &inputVec,
               OutputVec &outputVec,
               int multiThreadLevel) :
            client(&client), inputVec(&inputVec), outputVec(&outputVec),
            atomic_counter(), current_state(), thread_ptrs(),
            checkJoinMutex(PTHREAD_MUTEX_INITIALIZER), mapMutex(PTHREAD_MUTEX_INITIALIZER),
            reduceMutex(PTHREAD_MUTEX_INITIALIZER), outputVectorMutex(PTHREAD_MUTEX_INITIALIZER),
            updateCurrentMutex(PTHREAD_MUTEX_INITIALIZER),
            didJoinMutex(false), barrier(new Barrier(multiThreadLevel)){
        if (sem_init(&semaphore, 0, 1) != 0) {
            cout << ERROR_MSG << SEMAPHORE_CREATE_ERROR << endl;
            exit(EXIT_FAILURE);
        }
    }

    ~JobContext() {
        for (auto t : threadContextVec) {
            delete t->intermediateVec;
            delete t;
        }
        for (auto t : thread_ptrs) {
            delete t;
        }

        delete barrier;

        destroy_all_mutex();

        if (sem_destroy(&semaphore)!=0) {
            cout << ERROR_MSG <<(SEMAPHORE_DESTROY_ERROR) << endl;;
            exit(EXIT_FAILURE);
        }
    }

    void destroy_all_mutex() {
        if (pthread_mutex_destroy(&checkJoinMutex) != 0) {
            cout << ERROR_MSG << MUTEX_DESTROY_ERROR << endl;
            exit(EXIT_FAILURE);
        }

        if (pthread_mutex_destroy(&mapMutex) != 0) {
            cout << ERROR_MSG << MUTEX_DESTROY_ERROR << endl;
            exit(EXIT_FAILURE);
        }

        if (pthread_mutex_destroy(&reduceMutex) != 0) {
            cout << ERROR_MSG << MUTEX_DESTROY_ERROR << endl;
            exit(EXIT_FAILURE);
        }

        if (pthread_mutex_destroy(&outputVectorMutex) != 0) {
            cout << ERROR_MSG << MUTEX_DESTROY_ERROR << endl;
            exit(EXIT_FAILURE);
        }

        if (pthread_mutex_destroy(&updateCurrentMutex) != 0) {
            cout << ERROR_MSG << MUTEX_DESTROY_ERROR << endl;
            exit(EXIT_FAILURE);
        }
    }
};

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
    auto jobContextPtr = (JobContext *) context;
    auto outputPair = OutputPair(key, value);
    mutex_handler(Lock,&(jobContextPtr->outputVectorMutex));
    jobContextPtr->outputVec->push_back(outputPair);
    mutex_handler(Unlock,&(jobContextPtr->outputVectorMutex));
}

/**
 * a function gets JobHandle returned by startMapReduceFramework and waits until it is finished.
 * HINT - should use the c function pthread_join.
 * @param job the JobHandle we wait till is finished
 */
void waitForJob(JobHandle job) {
    auto *job_context = static_cast<JobContext*> (job);
    //check if it's the first time we call to this function with this jobhandle, if not return from function
    mutex_handler(Lock,&(job_context->checkJoinMutex));
    //todo: check if there another way to implement this
    //todo: check if I need tbool callShuffle;o check if the current state is Reduce and the percentage is 100
    if(job_context->didJoinMutex) {
        mutex_handler(Unlock,&(job_context->checkJoinMutex));
        return;
    }

    mutex_handler(Unlock,&(job_context->checkJoinMutex));
    mutex_handler(Lock,&(job_context->checkJoinMutex));
    job_context->didJoinMutex = true;
    mutex_handler(Unlock,&(job_context->checkJoinMutex));

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
    mutex_handler(Lock,&(job_context->updateCurrentMutex));
    state->stage = job_context->current_state.stage;

    switch (state->stage){
        case UNDEFINED_STAGE:
            state->percentage = 0;
            state->stage = UNDEFINED_STAGE;
            break;
        case MAP_STAGE:
            state->percentage = 100 * ((float) (static_cast<uint32_t>((job_context->atomic_counter->load() & 0x3FFFFFFF80000000) >> 31)
            ) / (float) job_context->inputVec->size());
            break;
        case SHUFFLE_STAGE:
            state->percentage = 100 * ((float) static_cast<uint32_t>(job_context->atomic_counter->load() & 0x7FFFFFFF) /
                    (float) job_context->total_pairs_amount);
            break;
        case REDUCE_STAGE:
            state->percentage = 100 * ((float) (static_cast<uint32_t>((job_context->atomic_counter->load() & 0x3FFFFFFF80000000) >> 31)
            ) /
                    (float) job_context->total_pairs_amount);
            break;
    }

    mutex_handler(Unlock,&(job_context->updateCurrentMutex));
}

void thread_map(ThreadContext* threadContext) {
    auto jobContext = threadContext->jobContextPtr;
    auto inputVector = jobContext->inputVec;
    mutex_handler(Lock, &(jobContext->mapMutex));

    if (jobContext->current_state.stage == UNDEFINED_STAGE) {
        jobContext->atomic_counter->fetch_add(static_cast<uint64_t>(1) << 62);
        jobContext->current_state.stage = MAP_STAGE;
    }

    unsigned long oldValue = jobContext->atomic_counter->fetch_add(1);
    oldValue = oldValue & 0x7FFFFFFF;

    mutex_handler(Unlock, &(jobContext->mapMutex));

    while(oldValue < inputVector->size()) {
        auto currentKey = inputVector->at(oldValue).first;
        auto currentVal = inputVector->at(oldValue).second;
        jobContext->client->map(currentKey, currentVal, threadContext);

        mutex_handler(Lock,&(jobContext->mapMutex));

        jobContext->atomic_counter->fetch_add(static_cast<uint64_t>(1) << 31);
//        cout << "counter incremented to " << static_cast<uint32_t>((jobContext->atomic_counter->load() & 0x3FFFFFFF80000000) >> 31) << endl;
        jobContext->current_state.percentage = 100 *
                ((float) (static_cast<uint32_t>((jobContext->atomic_counter->load() & 0x3FFFFFFF80000000) >> 31))
                / (float) inputVector->size());
        oldValue =  jobContext->atomic_counter->fetch_add(1);
        oldValue = oldValue & 0x7FFFFFFF;
        mutex_handler(Unlock,&(jobContext->mapMutex));
    }
}

bool comparator(IntermediatePair a1,IntermediatePair a2){
    return *a1.first < *a2.first;
}

IntermediatePair findTheMaxKey(ThreadContext* threadContext){
    IntermediatePair maxKey ;
    for(auto thread : threadContext->jobContextPtr->threadContextVec) {
        if (!thread->intermediateVec->empty()) {
            maxKey = thread->intermediateVec->back();
            break;
        }
    }
    for(auto thread : threadContext->jobContextPtr->threadContextVec){
        if(thread->intermediateVec->empty()){
            continue;
        }
        auto tmpThread = thread->intermediateVec->back();
        if(!comparator(tmpThread,maxKey)) {
            maxKey = tmpThread;
        }
    }
    return maxKey;
}

bool checkIfEqualMaxKey(IntermediatePair a, IntermediatePair b){
    bool a_smaller_then_b = *a.first < *b.first;
    bool b_smaller_then_a = *b.first < *a.first;
    return ! a_smaller_then_b && ! b_smaller_then_a;
}

void calculateShuffleCounter(ThreadContext* threadContext){
    mutex_handler(Lock,&(threadContext->jobContextPtr->mapMutex));
    if(threadContext->jobContextPtr->current_state.stage == MAP_STAGE){
        unsigned long int counter = 0;
        for (auto& thread: threadContext->jobContextPtr->threadContextVec) {
            counter += thread->intermediateVec->size();
        }
        *(threadContext->jobContextPtr->atomic_counter) = 0;
//        threadContext->jobContextPtr->atomic_counter->fetch_add(static_cast<uint64_t>(counter) << 31);
        threadContext->jobContextPtr->atomic_counter->fetch_add(static_cast<uint64_t>(2) << 62);
        threadContext->jobContextPtr->total_pairs_amount = counter;
        threadContext->jobContextPtr->current_state.stage = SHUFFLE_STAGE;
        threadContext->jobContextPtr->current_state.percentage = 0;
    }
    mutex_handler(Unlock,&(threadContext->jobContextPtr->mapMutex));
}

void shuffle(ThreadContext* threadContext) {
    //set the counter
    calculateShuffleCounter(threadContext);
    auto shuffleOutput = new vector<IntermediateVec>();
    auto job = threadContext->jobContextPtr;
    while(static_cast<uint32_t>(job->atomic_counter->load() & 0x7FFFFFFF) < job->total_pairs_amount) {
        IntermediateVec interVec;
        auto maxKey = findTheMaxKey(threadContext);
        //find the max key
        for(auto thread : threadContext->jobContextPtr->threadContextVec) {
            if(thread->intermediateVec->empty()){
                continue;
            }
            //check if the current value is equal to current max key
            while(checkIfEqualMaxKey(maxKey,thread->intermediateVec->back())){
                interVec.push_back(thread->intermediateVec->back());
                thread->intermediateVec->pop_back();
                //update counter
                job->atomic_counter->fetch_add(1);
                job->current_state.percentage = 100 * ((float) (job->atomic_counter->load() & 0x7FFFFFFF)/ (float)
                        job->total_pairs_amount);

                if(thread->intermediateVec->empty()) {
                    break;
                }
            }
        }
        shuffleOutput->push_back(interVec);
    }
    job->shuffleOutput = *shuffleOutput;
}


void thread_reduce(ThreadContext* threadContext) {
    auto jobContext = threadContext->jobContextPtr;
    mutex_handler(Lock, &(jobContext->reduceMutex));

    if (jobContext->current_state.stage == SHUFFLE_STAGE) {
        *(jobContext->atomic_counter) = 0;
        jobContext->atomic_counter->fetch_add(static_cast<uint64_t>(3) << 62);
        jobContext->current_state.stage = REDUCE_STAGE;
        jobContext->current_state.percentage = 0;
    }

    IntermediateVec current_pair_vector;

//    if (!threadContext->jobContextPtr->shuffleOutput.empty()){
//        current_pair_vector = threadContext->jobContextPtr->shuffleOutput.back();
//        threadContext->jobContextPtr->shuffleOutput.pop_back();
//    }
//    else{
//        mutex_handler(Unlock, &(threadContext->jobContextPtr->reduceMutex));
//        return;
//    }

    mutex_handler(Unlock, &(jobContext->reduceMutex));
    while(true) {
        mutex_handler(Lock, &(jobContext->reduceMutex));
        if(jobContext->shuffleOutput.empty()){
            mutex_handler(Unlock,&(jobContext->reduceMutex));
            return;
        }
        auto current_pair = jobContext->shuffleOutput.back();
        threadContext->jobContextPtr->shuffleOutput.pop_back();
        jobContext->client->reduce(&(current_pair), threadContext->jobContextPtr);
//        cout << threadContext << " === 2" << endl;


//        current_pair = threadContext->jobContextPtr->shuffleOutput.back();
//        threadContext->jobContextPtr->shuffleOutput.pop_back();

//        jobContext->shuffleOutput.pop_back();
        jobContext->atomic_counter->fetch_add(static_cast<uint64_t>(current_pair.size()) << 31);
//        cout << "counter incremented to " << static_cast<uint32_t>((jobContext->atomic_counter->load() & 0x3FFFFFFF80000000) >> 31) << endl;
        jobContext->current_state.percentage = 100 *
                                               ((float) (static_cast<uint32_t>((jobContext->atomic_counter->load() & 0x3FFFFFFF80000000) >> 31))
                                                / (float) jobContext->total_pairs_amount);
        mutex_handler(Unlock, &(jobContext->reduceMutex));
    }
}

/**
 * Releasing all resources of a job. You should prevent releasing resources before the job finished.
 * After this function is called the job handle will be invalid.
 * @param job - Job to release
 */
void closeJobHandle(JobHandle job) {
    waitForJob(job);
    delete static_cast<JobContext*>(job);
    job = nullptr;
}

void* single_thread_task(void* arg) {
    auto job = static_cast<JobContext*>(arg);
    auto _threadContext = new (nothrow) ThreadContext(job,job->barrier);
    if (_threadContext == nullptr) {
        cout << "yo44" << endl;
        exit(EXIT_FAILURE);
    }
    job->threadContextVec.push_back(_threadContext);
    //map
    thread_map(_threadContext);

    //sort
    sort(_threadContext->intermediateVec->begin(),_threadContext->intermediateVec->end(),comparator);

    //barrier
    _threadContext->jobContextPtr->barrier->barrier(); // TODO - remove comment (now it's here because this is not compiling)

    //shuffle
    if (sem_wait(&_threadContext->jobContextPtr->semaphore) != 0) {
        cout << ERROR_MSG << ERR_SEMAPHORE_WAIT << endl;
        exit(EXIT_FAILURE);
    }

    if(! _threadContext->jobContextPtr->callShuffle) {
        _threadContext->jobContextPtr->callShuffle = true;
        shuffle(_threadContext);
//    _threadContext->barrier->barrier();]
        for (int i = 0; i < _threadContext->jobContextPtr->threadContextVec.size(); ++i) {
            if (sem_post((&_threadContext->jobContextPtr->semaphore)) != 0) {
                cout << ERROR_MSG << ERR_SEMAPHORE_POST << endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    _threadContext->jobContextPtr->barrier->barrier(); // TODO - remove comment (now it's here because this is not compiling)

    //reduce
    thread_reduce(_threadContext);

    return nullptr;
}

JobHandle startMapReduceJob(const MapReduceClient& client,
                            const InputVec& inputVec,
                            OutputVec& outputVec,
                            int multiThreadLevel) {
    if (multiThreadLevel > 1000) { // TODO - find better solution
        cerr << ERROR_MSG << PTHREAD_CREATE_ERROR_MSG << endl;
        exit(FAILURE);
    }
    auto job = new (nothrow) JobContext(client, inputVec, outputVec, multiThreadLevel);
    job->atomic_counter = new (nothrow) atomic<uint64_t>(0);
    job->current_state.stage = UNDEFINED_STAGE;
    job->current_state.percentage = 0;

    for (int i = 0; i < multiThreadLevel; i++) {
//        cout << i << endl;
        auto new_thread = new pthread_t();
        job->thread_ptrs.push_back(new_thread);
        if (pthread_create(new_thread,
                           nullptr,
                           single_thread_task,
                           (void *) job) != 0) {
            cout << ERROR_MSG << PTHREAD_CREATE_ERROR_MSG << endl;
            exit(FAILURE);
        }
    }
    return static_cast<JobHandle>(job);
}

//todo: remove this function

pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;

void* single_thread(void* arg) {
    atomic<uint64_t>* counter = static_cast<atomic<uint64_t>*>(arg);
    mutex_handler(Lock,&mymutex);
    cout<<"hey"<<endl;
    (*counter)++;
    mutex_handler(Unlock,&mymutex);
    return NULL;
}

//
//int main(int argc, char** argv) {
//    CounterClient client;
//    InputVec inputVec;
//    OutputVec outputVec;
//
//    auto job = startMapReduceJob(client, inputVec, outputVec, 3);
//    waitForJob(job);
//    waitForJob(job);
//}