#include <deque>
#include <iostream>
#include "uthreads.h"

#define QUANTUM_ERROR "the quantum_usecs should be positive"

using namespace std;

enum states{
    READY,RUNNING,BLOCKING
};
class Threads{
private:
    states _states;
    int _id;
    char stack[STACK_SIZE];

public:
    Threads(states states,int id){
        _states = states;
        _id = id;

    }
    int get_id(){
        return _id;
    }

    states get_states(){
        return _states;
    }

    void change_states(states state){
        _states = state;
    }

};

static deque<Threads*> ready_queue(MAX_THREAD_NUM);
static int QUANTUM_USECS = 0;

int find_smallest_free_id(const deque<Threads*>& queue, int running_thread_index) {
    bool freeNumbers[100] = {false};

    for (auto thread : queue) {
        freeNumbers[thread->get_id()] = true;
    }

    freeNumbers[running_thread_index] = true;

    for (int i = 1; i < 101; i++) {
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
    auto threads_0 = new Threads(RUNNING,find_smallest_free_id(ready_queue,-1));
    running_thread = threads_0->get_id();
    return 0;


}



int main() {
    auto a = new Threads(READY, 1);
    auto b = new Threads(RUNNING, 2);
    auto c = new Threads(READY, 3);

    auto dq = new deque<Threads*>;
    dq->push_back(a);
    dq->push_back(c);

    cout << find_smallest_free_id(*dq, b->get_id()) << endl;
    return 1;
}





