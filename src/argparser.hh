#ifndef ARGPARSER
#define ARGPARSER

#include <string>
#include <stdexcept>
#include <iostream>

struct parsed_arguments {
    // path to the input file
    std::string input_file;
    // how many worker threads to be used
    unsigned int worker_count = 0;
    // how many rows to be used per chunk, 0 means default
    std::size_t row_count = 0;
};

[[noreturn]] void help(const char * const exe);

// Used to represent an error occurred while
// parsing the command line input, main errors are:
//  - required parameter missing
//  - invalid input parameters
class parsing_exception : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

// parse command line arguments and return an
// object easier to use in specific code
parsed_arguments parse(const int argc, const char * argv[]);


#endif
