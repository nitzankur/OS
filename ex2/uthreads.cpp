//
// Created by mikia on 18/04/2023.
//

#include <queue>
#include <iostream>

#define QUANTUM_ERROR "the quantum_usecs should be positive"

using namespace std;

static enum states{
    READY,RUNNING,BLOCKING
};
class Threads{
private:
    states _states;
    int _id;

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

static std::queue<Threads*> ready_queue;
static int QUANTUM_USECS = 0;



int uthread_init(int quantum_usecs){
    if (quantum_usecs < QUANTUM_USECS){
        std::cerr <<QUANTUM_ERROR<<std::endl;
        return 1;
    }
    QUANTUM_USECS = QUANTUM_USECS;
    return 0;
//    Threads threads_0 = new Threads(states.READY);

}





