/**
 *  Test calculus of cross correlation
 */

#include "../modules/CPP-test-unit/tester.hh"
#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"

#include "../src/chunk.hh"
#include "../src/numeric_parser.hh"
#include "../src/cuda_numeric_consumer.hh"

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <valarray>
#include <iostream>
#include <algorithm>

template <typename T>
std::valarray<T> evaluate(const std::vector<std::vector<T>>& dataset) {
    auto columns = dataset.size();
    auto couples = columns*(columns-1)/2;
    std::valarray<T> ans(couples);
    int pairIdx{};
    for (int i{}; i+1!=columns; ++i) {
        for (int j{i+1}; j!=columns; ++j) {
            ans[pairIdx++] += math::statistics::pearson_correlation_coefficient(dataset[i], dataset[j]).compute();
        }
    }
    return ans;
}


/**
 * @brief Test if the explicit calculus give the same result of the
 * one performed by the class numeric_consumer
 * 
 * @return tester 
 */
tester test_xcorr([](){
    using test_type = double;
    constexpr std::size_t rows = 1000;
    constexpr std::size_t cols = 30;

    // initialize random number generator
    std::default_random_engine generator;
    std::uniform_real_distribution<test_type> distribution(30,77);

    // generate matrix
    std::vector<std::vector<test_type>> dataset; dataset.reserve(cols);
    for (int c{}; c!=cols; ++c) {
        std::vector<test_type> column; column.reserve(rows);
        for (int r{}; r!=rows; ++r) {
            column.push_back(distribution(generator));
        }
        dataset.push_back(std::move(column));
    }

    // generate queue:
    //  INPUT queue with chunk to be parsed
    auto chunkQueue = std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<test_type>>>(
        new lockfree_queue::fixed_size_lockfree_queue<chunk<test_type>>(1)
    );

    // generate chunk
    chunk<test_type> cnk(rows, cols);
    for (int r{}; r!=rows; ++r) {
        for (int c{}; c!=cols; ++c) {
            cnk.push_back(dataset[c][r]);
            if (cnk.at(r,c) != dataset[c][r]) {
                throw std::logic_error("Strange chunk insertion.");
            }
        }
    }

    // push chunk in queue
    auto cnk_ptr = std::make_unique<decltype(cnk)>(std::move(cnk));
    if (!chunkQueue->offer(cnk_ptr)) {
        throw std::logic_error("Failed chunk insertion.");
    }

    cuda_numeric_consumer<test_type> consumer(cols, chunkQueue);

    // analyze results
    consumer.analyze();

    auto res = consumer.get_results_and_invalidate();
    std::valarray<test_type> casted(res.size());
    std::transform(std::begin(res), std::end(res), std::begin(casted),
        [](decltype(res[0]) it){
            return it.compute();
        });

    auto evaluated = evaluate(dataset);
    // check equivalence
    if ((casted != evaluated).min() == false) {
        std::cerr << "casted | evaluated:\n";
        int idx{};
        for (int i{}; i+1!=cols; ++i) {
            for (int j{i+1}; j!=cols; ++j) {
                using namespace std::literals;
                std::cerr << " (" << i << "," << j << ") " <<
                    casted[idx] << " | " << evaluated[idx] << '\n';
            }
        }

        throw std::logic_error("Failed analysis.");
    }

});