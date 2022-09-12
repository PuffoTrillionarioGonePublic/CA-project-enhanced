/**
 *  Test results of class reader
 */

#include "../modules/CPP-test-unit/tester.hh"
#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"
#include "../modules/CPP-math-utils/convertions.hh"

#include "../src/chunk.hh"
#include "../src/reader.hh"

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <valarray>
#include <iostream>
#include <algorithm>


tester test_reader([](){
    // file containing inputs
    const std::string test_file = "test.csv";
    // size of the queue containing the rows produced
    // by the reader, set to the size of the test file
    constexpr std::size_t rows = 10;
    // forst value in the dataset: 0
    constexpr int initial_value{};

    // generate queue:
    //  OUTPUT queue with strings to read
    auto outQueue = std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>>(
        new lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>(rows)
    );

    // define reader
    reader r(test_file, outQueue);

    // read all input
    if (!r.consume_many()) {
        // input file is dimensioned to get true from consume_many()
        throw std::logic_error("Error in row consuming!");
    }

    // outQueue size should be coherent
    if (!outQueue->full()) {
        throw std::logic_error("outQueue should be full!");
    }

    // test that all output number are progressive and start from 0
    std::unique_ptr<std::vector<std::string>> row;
    std::size_t read_rows{};
    int expected_value{initial_value};
    while (outQueue->poll(row)) {
        // repeat untill new rows can be extracted
        ++read_rows;

        // rest row content
        for (const auto& x : *row) {
            if (math::convertions::ston<decltype(expected_value)>(x) != expected_value) {
                using namespace std::literals;
                throw std::logic_error("Found "s + x + " instead of " + std::to_string(expected_value));
            }
            ++expected_value;
        }

        // free data
        row.release();
    }

    if (read_rows != outQueue->capacity()) {
        // mess!
        throw std::logic_error("Error while reading outQueue!");
    }
});
