#include <deque>
#include <iostream>
#include "uthreads.h"
#include <setjmp.h>
#include <signal.h>
#include <bits/sigaction.h>

#define QUANTUM_ERROR "the quantum_usecs should be positive"
#define BLOCKED_THE_MAIN_THREAD "can't blocked the main thread"
#define JB_SP 6
#define JB_PC 7

using namespace std;

enum states{
    READY,RUNNING,BLOCKING
};

enum scheduling_states{
    BLOCKED
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
        return 0;
    }
};

static deque<Thread*> ready_queue(MAX_THREAD_NUM);
static deque<Thread*> blocked_queue(MAX_THREAD_NUM);
static Thread* running_thread;
static int QUANTUM_USECS = 0;
static struct sigaction sa;
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
    return -1;
}

int uthread_init(int quantum_usecs){
    if (quantum_usecs < QUANTUM_USECS){
        std::cerr <<QUANTUM_ERROR<<std::endl;
        return -1;
    }
    QUANTUM_USECS = QUANTUM_USECS;
    auto threads_0 = new Thread(RUNNING, 0);
//    running_thread = threads_0->get_id();
    return 0;
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
    if (new_thread_id == -1) {
        cerr << "thread library error: Threads amount exceeded the max thread amount limit" << endl;
    }

    // TODO - How to use entry point?
    auto new_thread = new Thread(READY, new_thread_id);
    ready_queue.push_back(new_thread); // TODO - Is this what i should do?
}
int sleep(){
    //TODO - implement sleep function
    return 0;
}

int round_robin_scheduling(scheduling_states schedulingStates){

    switch(schedulingStates){
        case BLOCKED:
            sleep();
            uthread_resume( blocked_queue.front()->get_id())
            Thread *current_thread = blocked_queue.front();
            blocked_queue.pop_front();
            ready_queue.push_back(current_thread);
            break;
            //current_time +=

    }
    return 0;

}

int uthread_resume(){

}

int uthread_block(int tid){
    // if thread tid == 0
    if(tid==0){
        std::cerr<<"BLOCKED_THE_MAIN_THREAD"<<endl;
        return -1;
    }
    //if thread blocking himself
    if(thread_blocking_himself){

    }
    //block the signal
    sigprocmask(SIG_BLOCK,&set,NULL);
}

int uthread_sleep(int num_quantums){

    return 0;
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





