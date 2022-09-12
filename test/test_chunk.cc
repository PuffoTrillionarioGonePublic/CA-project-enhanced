/**
 *  Test 
 */

#include "../modules/CPP-test-unit/tester.hh"

#include "../src/chunk.hh"

#include <stdexcept>
#include <random>
#include <string>


tester test_empty([](){
    using test_type = int;
    constexpr std::size_t rows = 10;
    constexpr std::size_t cols = 20;

    // check empty
    chunk<test_type> c(rows, cols);
    if (!c.empty()) {
        throw std::logic_error("Chunk should be empty");
    }
    if (c.full()) {
        throw std::logic_error("Chunk should not be full");
    }
});


tester t1([](){
    using test_type = double;
    constexpr std::size_t rows = 2;
    constexpr std::size_t cols = 3;

    test_type test_mat[rows][cols] {};
    chunk<test_type> test_cnk(rows, cols);

    // initialize random number generator
    std::default_random_engine generator;
    std::uniform_real_distribution<test_type> distribution(30,77);

    // fill mat and chunk by rows
    // for each row
    for (std::size_t r{}; r!=rows; ++r) {
        // for each col
        for (std::size_t c{}; c!=cols; ++c) {
            auto tmp = distribution(generator);
            test_mat[r][c] = tmp;
            test_cnk.push_back(tmp);
        }
    }
    // should be full
    if (!test_cnk.full()) {
        throw std::logic_error("Chunk should be full");
    }
    // compare values
    for (std::size_t r{}; r!=rows; ++r) {
        // for each col
        for (std::size_t c{}; c!=cols; ++c) {
            if (test_mat[r][c] != test_cnk.at(r,c)) {
                using namespace std::literals;
                throw std::logic_error("Inequality found at ("s + std::to_string(r) + ","s + std::to_string(c) + ")"s);
            }
        }
    }
});
