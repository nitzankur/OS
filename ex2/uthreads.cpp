#include <deque>
#include <iostream>
#include "uthreads.h"
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <bits/sigaction.h>
#include <sys/time.h>

#define QUANTUM_ERROR "the quantum_usecs should be positive"
#define BLOCKED_THE_MAIN_THREAD "can't blocked the main thread"
#define NUMBER_THREAD_ID_IS_NOT_VALID "number thread id is not valid"
#define MAX_NUMBER_OF_THREADS " Threads amount exceeded the max thread amount limit"
#define THREAD_LIBRARY_ERROR "thread library error: "
#define SYSTEM_ERROR "system error: "
#define NULL_ENTRY_POINT_ERROR "Entry point is null"
#define SET_TIMER_ERROR "setitimer error."
#define SIGACTION_ERROR "sigaction error."
#define THREAD_GET_QUANTUM_ERROR "thread's get quantum error - no thread with the requested ID"
#define MAIN_THREAD_SLEEP_ERROR "trying to call sleep() on the main thread"
#define NON_POSITIVE_NUMBER_OF_SLEEP_QUANTUMS_ERROR "sleep quantums number passed to uthread_sleep() is non positive"
#define NO_FREE_ID_ERROR "no free id found"
#define MEMORY_ALLOCATION_FAILED_ERROR "memory allocation failed"
#define SUCCESS 0
#define FAILURE -1
#define JB_SP 6
#define JB_PC 7
#define MICROSECONDS_IN_SECOND 1000000
#define EXIT_CODE 1

using namespace std;

enum states{
    READY,RUNNING,BLOCKING
};

enum scheduling_states{
    BLOCKED_HIMSELF=1,QUANTUM_EXPIRED=26,TERMINATE=3
};

typedef unsigned long address_t;

class Thread {
 private:
    states _states;
    int _id;
    char stack[STACK_SIZE];
    int quantums = 0;
    sigjmp_buf env;
    int quantums_till_wake_up;


 public:
    Thread(states states, int id) {
        _states = states;
        _id = id;

    }
    int get_id(){
        return _id;
    }

    int get_quantums(){
        return quantums;
    }


    states get_states(){
        return _states;
    }

    char* get_stack() {
        return stack;
    }

    sigjmp_buf& get_env() {
        return env;
    }

    void change_states(states state){
        _states = state;
    }

    void increase_quantums(){
        quantums++;
    }

    float time_remaining(){
        //todo: understand how we follow the time of the thread
        return SUCCESS;
    }

    void decrease_quantums_till_wake_up() {
        quantums_till_wake_up--;
    }

    void set_quantums_till_wake_up(int new_value) {
        quantums_till_wake_up = new_value;
    }

    int get_quantums_till_wake_up() {
        return quantums_till_wake_up;
    }
};

static deque<Thread*> ready_queue;
static deque<Thread*> blocked_queue;
static Thread* running_thread;
static int quantum_ticks_counter = 1;
struct sigaction quantum_tick_action;
static int program_quantum_usecs = 0;
struct itimerval program_timer;
static sigset_t set;

//====help function====
void unblock_helper(){
    sigprocmask(SIG_UNBLOCK,&set,nullptr);
}

void block_helper(){
    sigprocmask(SIG_BLOCK,&set,nullptr);
}

address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

int delete_thread_from_queue(int tid, bool ready) {
    int current_index = 0;
    std::deque<Thread*> queue_to_remove;
    if(ready){
        queue_to_remove = ready_queue;
    }
    else{
        queue_to_remove = blocked_queue;
    }
    for (auto thread : queue_to_remove) {
        if (thread->get_id() == tid) {
            queue_to_remove.erase(queue_to_remove.begin() + current_index);
            return SUCCESS;
        }
        current_index++;
    }

    return FAILURE;
}

void delete_all_threads() {
    for (auto thread : ready_queue) {
        delete_thread_from_queue(thread->get_id(), true);
        delete(thread);
        thread = nullptr;
    }

    for (auto thread : blocked_queue) {
        delete_thread_from_queue(thread->get_id(), false);
        delete(thread);
        thread = nullptr;
    }

    delete running_thread;
    running_thread = nullptr;
}

int find_smallest_free_id() {
    auto running_thread_index = running_thread->get_id();

    bool freeNumbers[MAX_THREAD_NUM] = {false};

    for (auto thread : ready_queue) {
        freeNumbers[thread->get_id()] = true;
    }

    for (auto thread : blocked_queue) {
        freeNumbers[thread->get_id()] = true;
    }

    freeNumbers[running_thread_index] = true;

    for (int i = 1; i < MAX_THREAD_NUM; i++) {
        if (!freeNumbers[i]) {
            return i;
        }
    }

    return FAILURE;
}

Thread* find_thread(int tid) {
    for (auto thread : ready_queue) {
        if(thread->get_id() == tid){
            return thread;
        }
    }

    return NULL;

}

void setup_thread(Thread* thread, thread_entry_point entry_point) {

    // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
    // siglongjmp to jump into the thread.
    auto sp = (address_t) thread-> get_stack() + STACK_SIZE - sizeof(address_t);
    auto pc = (address_t) entry_point;
    sigsetjmp(thread->get_env(), 1); // Assign the environment the thread will return to when using sislongjmp
    (thread->get_env()->__jmpbuf)[JB_SP] = translate_address(sp); // assign the SP of the env
    (thread->get_env()->__jmpbuf)[JB_PC] = translate_address(pc); // assign the PC of the env
    sigemptyset(&thread->get_env()->__saved_mask); // clears the signal mask of the env, which means that no signal is blocked from interrupting the thread
}





void uthred_running() {
    Thread* next_thread = ready_queue.front();
    next_thread->change_states(RUNNING);
    next_thread->increase_quantums();
    running_thread = next_thread;
    ready_queue.pop_front();
    unblock_helper();
    siglongjmp(next_thread->get_env(),1);
}

void update_sleep() {
    block_helper();
    for (auto it = blocked_queue.begin(); it != blocked_queue.end(); it++) {
        if ((*it)->get_quantums_till_wake_up() > 0) { // Is sleeping
            (*it)->decrease_quantums_till_wake_up();
            if ((*it)->get_quantums_till_wake_up() == 0) {
                ready_queue.push_back(*it);
                blocked_queue.erase(it);
            }
        }
    }

    unblock_helper();
}

void round_robin_scheduling(int schedulingStates) {
    printf("yo");
    update_sleep();
    block_helper();
    quantum_ticks_counter++;
    int ret_val;
    switch(schedulingStates) {
        case BLOCKED_HIMSELF:
            blocked_queue.push_back(running_thread);
            running_thread->change_states(BLOCKING);
            if (setitimer(ITIMER_VIRTUAL, &program_timer, NULL)) {
                cerr << SYSTEM_ERROR << SET_TIMER_ERROR << endl;
                delete_all_threads();
                exit(EXIT_CODE);
            }
            ret_val = sigsetjmp(running_thread->get_env(), 1);
            if(ret_val == 0){
                uthred_running();
            }
            break;
        case QUANTUM_EXPIRED:
            running_thread->change_states(READY);
            ready_queue.push_back(running_thread);
            ret_val = sigsetjmp(running_thread->get_env(), 1);
            if(ret_val == 0){
                uthred_running();
            }
            break;
        case TERMINATE:
            if (setitimer(ITIMER_VIRTUAL, &program_timer, NULL)) {
                cerr << SYSTEM_ERROR << SET_TIMER_ERROR << endl;
                delete_all_threads();
                exit(EXIT_CODE);

            }
            uthred_running();
            break;
    }

}



int initialize_global_timer(int quantum_usecs) {
//    unblock_helper();
    sigemptyset(&quantum_tick_action.sa_mask);

    quantum_tick_action.sa_handler = &round_robin_scheduling;

    if (sigaction(SIGVTALRM, &quantum_tick_action, nullptr) < 0) {
        cerr << THREAD_LIBRARY_ERROR << SIGACTION_ERROR << endl;
        delete_all_threads();
//        unblock_helper();
        exit(EXIT_CODE);
    }

    if (quantum_usecs < 0) {
        std::cerr <<QUANTUM_ERROR<<std::endl;
        return FAILURE;
    }

    program_timer.it_value.tv_sec = quantum_usecs / MICROSECONDS_IN_SECOND;
    program_timer.it_value.tv_usec = quantum_usecs % MICROSECONDS_IN_SECOND;

    program_timer.it_interval.tv_sec = quantum_usecs / MICROSECONDS_IN_SECOND;
    program_timer.it_interval.tv_usec = quantum_usecs % MICROSECONDS_IN_SECOND;

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &program_timer, nullptr)) {
        cerr << SYSTEM_ERROR << SET_TIMER_ERROR << endl;
        delete_all_threads();
//        unblock_helper();
        exit(EXIT_CODE);
    }



    return SUCCESS;
}
//====library function====


int uthread_init(int quantum_usecs) {
//    block_helper();
    running_thread = new (nothrow) Thread(RUNNING, 0);
    if (running_thread == nullptr) {
        cerr << SYSTEM_ERROR << MEMORY_ALLOCATION_FAILED_ERROR << endl;
//        unblock_helper();
        exit(EXIT_FAILURE);
    }

    sigemptyset(&set);
    sigaddset(&set, SIGVTALRM);

    if (initialize_global_timer(quantum_usecs) == FAILURE) {
//        unblock_helper();
        return FAILURE;
    }
//    unblock_helper();
    return SUCCESS;
}

int uthread_spawn(thread_entry_point entry_point) {
    block_helper();
    if (entry_point == nullptr) {
        cerr << THREAD_LIBRARY_ERROR << NULL_ENTRY_POINT_ERROR << endl;
        delete_all_threads();
        unblock_helper();
        return FAILURE;
    }

    auto new_thread_id = find_smallest_free_id();
    if (new_thread_id == FAILURE) {
        unblock_helper();
        return FAILURE;
    }

    auto new_thread = new (nothrow) Thread(READY, new_thread_id);
    if (running_thread == nullptr) {
        cerr << SYSTEM_ERROR << MEMORY_ALLOCATION_FAILED_ERROR << endl;
        unblock_helper();
        exit(EXIT_FAILURE);
    }

    setup_thread(new_thread, entry_point);
    ready_queue.push_back(new_thread);
//    unblock_helper();
    return new_thread_id;
}

int uthread_terminate(int tid) {
    block_helper();
    auto thread = find_thread(tid);
    if(thread == NULL){
        std::cerr<<THREAD_LIBRARY_ERROR<<NUMBER_THREAD_ID_IS_NOT_VALID<<endl;
        unblock_helper();
        return FAILURE;
    }
    if (tid == 0) {
        delete_all_threads();
        unblock_helper();
        exit(EXIT_SUCCESS);
    }
    switch (thread->get_states()){
        case READY:
            delete_thread_from_queue(tid, true);
            delete(thread);
            break;
        case BLOCKING:
            delete_thread_from_queue(tid, false);
            delete(thread);
            break;
        case RUNNING:
            running_thread = nullptr;
            delete(thread);
            unblock_helper();
            round_robin_scheduling(TERMINATE);
    }
    unblock_helper();
    return SUCCESS;
}

int uthread_block(int tid) {
    block_helper();
    if(tid==0){
        std::cerr<<THREAD_LIBRARY_ERROR<<BLOCKED_THE_MAIN_THREAD<<endl;
        unblock_helper();
        return FAILURE;
    }

    Thread* thread = find_thread(tid);
    if(tid<SUCCESS||tid > MAX_THREAD_NUM || thread == NULL) {
        std::cerr<<THREAD_LIBRARY_ERROR<<NUMBER_THREAD_ID_IS_NOT_VALID<<endl;
        unblock_helper();
        return FAILURE;
    }
    //if thread blocking himself
    if(tid==running_thread->get_id() ) {
        unblock_helper();
        round_robin_scheduling(BLOCKED_HIMSELF);
    }
    // if thread ia already in block state
    if(thread->get_states() == BLOCKING){
        unblock_helper();
        return SUCCESS;
    }
    thread->change_states(BLOCKING);
    blocked_queue.push_back(thread);
    unblock_helper();
    return SUCCESS;
}

int uthread_resume(int tid) {
    block_helper();
    Thread* thread = find_thread(tid);
    if(thread == NULL) {
        std::cerr<<THREAD_LIBRARY_ERROR<<NUMBER_THREAD_ID_IS_NOT_VALID<<endl;
        unblock_helper();
        return FAILURE;
    }

    if (thread->get_states() == BLOCKING){
        blocked_queue.pop_front();
        thread->change_states(READY);
        ready_queue.push_back(thread);
    }
    unblock_helper();
    return SUCCESS;
}

int uthread_sleep(int num_quantums) {
    block_helper();
    if (running_thread->get_id() == 0) {
        cerr << THREAD_LIBRARY_ERROR << MAIN_THREAD_SLEEP_ERROR << endl;
        unblock_helper();
        return FAILURE;
    }

    if (num_quantums <= 0) {
        cerr << THREAD_LIBRARY_ERROR << NON_POSITIVE_NUMBER_OF_SLEEP_QUANTUMS_ERROR << endl;
        unblock_helper();
        return FAILURE;
    }

    running_thread->set_quantums_till_wake_up(num_quantums);
    unblock_helper();
    round_robin_scheduling(BLOCKED_HIMSELF);
    unblock_helper();
    return SUCCESS;
}

int uthread_get_tid() {
    return running_thread->get_id();
}

int uthread_get_total_quantums() {
//    sigset_t new_set;
//    sigemptyset(&new_set);
//    sigpending(&new_set);
    return quantum_ticks_counter;
}

int uthread_get_quantums(int tid) {
    auto thread = find_thread(tid);
    if (thread == NULL) {
        cerr << THREAD_LIBRARY_ERROR << THREAD_GET_QUANTUM_ERROR << endl;
        return FAILURE;
    }

    return thread->get_quantums();
}

