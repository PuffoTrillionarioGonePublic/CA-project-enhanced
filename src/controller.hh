
#ifndef CONTROLLER
#define CONTROLLER

#include <vector>
#include <string>
#include <thread>

#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"

#include "chunk.hh"

// Floating point type to be used
template <typename T>
class controller {

    // lock free queue containing vectors of strings to be
    // converted to floating points
    lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>> rows;

};


#endif
