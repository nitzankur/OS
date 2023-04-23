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
#define SUCCESS 0
#define FAILURE -1
#define JB_SP 6
#define JB_PC 7

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


public:

    Thread(states states, int id){
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

    void set_quantums(){
        quantums++;
    }

    float time_remaining(){
        //todo: understand how we follow the time of the thread
        return SUCCESS;
    }
};

static deque<Thread*> ready_queue(MAX_THREAD_NUM);
static deque<Thread*> blocked_queue(MAX_THREAD_NUM);
static Thread* running_thread;
static int quantum_ticks_counter = 1;
struct sigaction quantum_tick_action;
static int program_quantum_usecs = 0;
struct itimerval program_timer;
static sigset_t set;

address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
            : "=g" (ret)
            : "0" (addr));
    return ret;
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

    cout << "Failed to find a free ID" << endl;
    return FAILURE;
}

int uthread_init(int quantum_usecs){
    if (quantum_usecs < QUANTUM_USECS){
        std::cerr <<QUANTUM_ERROR<<std::endl;
        return FAILURE;
    }
    QUANTUM_USECS = QUANTUM_USECS;
    auto threads_SUCCESS = new Thread(RUNNING, SUCCESS);
//    running_thread = threads_SUCCESS->get_id();
    return SUCCESS;
}

void setup_thread(Thread* thread, char *stack, thread_entry_point entry_point)
{
    // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
    // siglongjmp to jump into the thread.
    auto sp = (address_t) thread-> get_stack(); // TODO - How do we update the position at the stack? And why is this different then in the jmp code example?
    auto pc = (address_t) entry_point;
    sigsetjmp(thread->get_env(), 1); // Assign the environment the thread will return to when using sislongjmp
    (thread->get_env()->__jmpbuf)[JB_SP] = translate_address(sp); // assign the SP of the env
    (thread->get_env()->__jmpbuf)[JB_PC] = translate_address(pc); // assign the PC of the env
    sigemptyset(&thread->get_env()->__saved_mask); // clears the signal mask of the env, which means that no signal is blocked from interrupting the thread
}

int uthread_spawn(thread_entry_point entry_point) {
    auto new_thread_id = find_smallest_free_id();
    if (new_thread_id == FAILURE) {
        cerr << "thread library error: Threads amount exceeded the max thread amount limit" << endl;
    }

    // TODO - How to use entry point?
    auto new_thread = new Thread(READY, new_thread_id);
    ready_queue.push_back(new_thread); // TODO - Is this what i should do?
}
int sleep(){
    //TODO - implement sleep function
    return SUCCESS;
}

void uthred_running() {
    Thread* next_thread = ready_queue.front();
    next_thread->change_states(RUNNING);
    next_thread->increase_quantums();
    running_thread = next_thread;
//    initialize_global_timer(program_quantum_usecs);
    //TODO: SET PROCMASK
    ready_queue.pop_front();
    siglongjmp(next_thread->get_env(),1);
}


void round_robin_scheduling(int schedulingStates) {
    int ret_val;
    switch(schedulingStates){
        case BLOCKED_HIMSELF:
            uthread_sleep(running_thread->get_id());
            delete_thread_from_ready_queue(running_thread->get_id());
            uthread_resume( blocked_queue.front()->get_id());
            ret_val = sigsetjmp(running_thread->get_env(), 1);
            if(ret_val == 0){
                uthred_running();
            }
            break;
        case QUANTUM_EXPIRED:
            delete_thread_from_ready_queue(running_thread->get_id());
            ready_queue.push_back(running_thread);
            ret_val = sigsetjmp(running_thread->get_env(), 1);
            if(ret_val == 0) {
                uthred_running();
            }
            break;
        case TERMINATE:
        default:


            break;

    }
}

int initialize_global_timer(int quantum_usecs) {
    if (quantum_usecs < 0) {
        std::cerr <<QUANTUM_ERROR<<std::endl;
        return FAILURE;
    }

    program_timer.it_interval.tv_sec = quantum_usecs / MICROSECONDS_IN_SECOND;
    program_timer.it_interval.tv_usec = quantum_usecs % MICROSECONDS_IN_SECOND;
    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &program_timer, NULL)) {
        cerr << THREAD_LIBRARY_ERROR << SET_TIMER_ERROR << endl;
        return FAILURE;
    }

    quantum_tick_action.sa_handler = &round_robin_scheduling;

    if (sigaction(SIGVTALRM, &quantum_tick_action, NULL) < 0) {
        cerr << THREAD_LIBRARY_ERROR << SIGACTION_ERROR << endl;
        return FAILURE;
    }

    return SUCCESS;
}


int uthread_init(int quantum_usecs) {
    if (initialize_global_timer(quantum_usecs) == FAILURE) {
        return FAILURE;
    }

    auto threads_SUCCESS = new Thread(RUNNING, SUCCESS);
//    running_thread = threads_SUCCESS->get_id();
    return SUCCESS;
}


int uthread_resume(int tid){
    Thread* thread = find_the_thread(tid);
    if(thread == NULL){
        std::cerr<<THREAD_LIBRARY_ERROR<<NUMBER_THREAD_ID_IS_NOT_VALID<<endl;
        return FAILURE;
    }
    if (thread->get_states() == BLOCKING){
        blocked_queue.pop_front();
        thread->change_states(READY);
        ready_queue.push_back(thread);
    }
    return SUCCESS;
}



void unblock_helper(){

}

void block_helper(){
    sigprocmask(SIG_UNBLOCK,&set,NULL);
}


int uthread_block(int tid){
    if(tid==0){
        std::cerr<<THREAD_LIBRARY_ERROR<<BLOCKED_THE_MAIN_THREAD<<endl;
        return FAILURE;
    }
    Thread* thread = find_the_thread(tid);
    if(tid<SUCCESS||tid > MAX_THREAD_NUM || thread == NULL){
        std::cerr<<THREAD_LIBRARY_ERROR<<NUMBER_THREAD_ID_IS_NOT_VALID<<endl;
        return FAILURE;
    }
    //if thread blocking himself
    if(tid==running_thread->get_id() ){
        round_robin_scheduling(BLOCKED_HIMSELF);
        return SUCCESS;
    }
    // if thread ia already in block state
    if(thread->get_states() == BLOCKING){
        unblock_helper();
        return SUCCESS;
    }
    //TODO: understand when to do sigblock and when unblock
    sigprocmask(SIG_BLOCK,&set,NULL);
    return SUCCESS;
}

int uthread_sleep(int num_quantums){

    return SUCCESS;
}


int uthread_get_tid(){
    return running_thread->get_id();
}



int main() {
    auto a = new Thread(READY, 1);
    auto b = new Thread(RUNNING, 2);
    auto c = new Thread(READY, 3);

    auto dq = new deque<Thread*>;
    dq->push_back(a);
    dq->push_back(c);

    cout << find_smallest_free_id() << endl;
    return 1;
}





