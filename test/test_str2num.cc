/**
 *  Test conversion from string to double
 */

#include "../modules/CPP-test-unit/tester.hh"
#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"

#include "../src/chunk.hh"
#include "../src/numeric_parser.hh"

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief test if the convertion between string and numeric
 * types occur correctly
 * 
 * @return tester 
 */
tester test_conversion_int([](){
    using test_type = int;
    constexpr std::size_t rows = 10;
    constexpr std::size_t cols = 20;

    // for comparison with parsing results
    test_type test_mat[rows][cols] {};
    test_type generator{};

    // generate queues:
    //  INPUT queue with strings to read
    auto inQueue = std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>>(
        new lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>(rows)
    );
    //  OUTPUT queue with the resulting chunk
    auto outQueue = std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<test_type>>>(
        new lockfree_queue::fixed_size_lockfree_queue<chunk<test_type>>(1)
    );

    // to get result
    std::unique_ptr<chunk<test_type>> result_chunk;

    // numeric parser to be tested
    numeric_parser<test_type> np(cols, inQueue, outQueue);

    // fill INPUT QUEUE
    for (std::size_t r{}; r!=rows; ++r) {
        // line to parse
        std::vector<std::string> vs;
        for (std::size_t c{}; c!=cols; ++c) {
            vs.push_back(std::to_string(generator));
            test_mat[r][c] = generator++;
        }
        // store vector
        auto offered = std::make_unique<decltype(vs)>(std::move(vs));
        if (!inQueue->offer(offered)) {
            throw std::logic_error("Failed insertion in inQueue");
        }
    }

    // check for correct inQueue size
    if (inQueue->size() != rows) {
        throw std::logic_error("Bad size of inQueue");
    }

    // check right size for output queue
    if (!outQueue->empty()) {
        throw std::logic_error("Bad size of inQueue");
    }

    // run parser
    // parse all available rows
    np.parse_many();

    // check right size for output queue
    if (!outQueue->empty()) {
        throw std::logic_error("Bad size of outQueue after parsing: should be 0");
    }

    // force output of new chunk
    np.store_partial_chunk();

    // now outQueue should contain exactly one element
    if (outQueue->size() != 1) {
        throw std::logic_error("Bad size of outQueue: should be 1");
    }

    // extract output chunk
    if (!outQueue->poll(result_chunk)) {
        throw std::logic_error("Failed extraction from output queue");
    }
    // and test it
    // check size
    if (result_chunk->rows() != rows || result_chunk->cols() != cols) {
        throw std::logic_error("Error in chunk size");
    }
    // check content
    for (std::size_t r{}; r!=rows; ++r) {
        for (std::size_t c{}; c!=cols; ++c) {
            if (test_mat[r][c] != result_chunk->at(r,c)) {
                using namespace std::literals;
                throw std::logic_error("Inequality found at ("s + std::to_string(r) + ","s + std::to_string(c) + ")"s);
            }
        }
    }


});

