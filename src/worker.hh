
#ifndef WORKER
#define WORKER

// to communicate with other thread
#include "queues.hh"
// to parse string
#include "numeric_parser.hh"
// to analyze results
#include "numeric_consumer.hh"


#include <valarray>
#include <thread>
#include <random>

// type of data output
template <typename T, template<typename> typename _numeric_consumer = numeric_consumer>
class worker
{
public:
    static std::size_t result_size_from_column_count(std::size_t column_count) {
        return column_count*(column_count-1)/2;
    }

private:
    const std::size_t column_count;
    std::shared_ptr<queues<T>> data_queues;

    // classes effectively performing computations
    numeric_parser<T> parser;
    _numeric_consumer<T> analyser;

    // to prevent logi errors
    bool parse_guard{};
    bool compute_guard{};

#ifdef YIELD
#pragma message "Yield processor if thread 'stalls'..."
    // did nothing during the last iteration?
    bool stalled{};
#endif

    // to handle worker in separate thread
    std::thread worker_thread;

    // to grant a minimal amount of randomness in the project
    // i.e. prevent all threads to continuosly try to use the
    // same resources concurrently
    std::unique_ptr<std::random_device> dev; // only one device for all workers
    std::minstd_rand0 rng;
    std::uniform_int_distribution<int> distribution; // distribution in range [1, 6]

    int parse_repetitions;
    int compute_repetitions;

public:
    worker(std::size_t column_count, std::shared_ptr<queues<T>> data_queues)
    : column_count{column_count},
      data_queues{std::move(data_queues)},
      parser(column_count, this->data_queues->rowQueue, this->data_queues->chunkQueue),
      analyser(column_count, this->data_queues->chunkQueue),
      distribution(1,6)
    {
        if (!dev) {
            dev = std::unique_ptr<std::random_device>(new std::random_device());
        }
        rng = std::minstd_rand0((*dev)());
        if (this->data_queues->rows_per_chunk) {
            // specify chunk size different from the defaul
            parser.set_rows_per_chunk(this->data_queues->rows_per_chunk);
        }
        // reduce calls to random function
        this->parse_repetitions = distribution(rng);
        this->compute_repetitions = 1+distribution.max()-parse_repetitions;
    }

    ~worker() = default;

    // to perform parsing - stop after one chunk is completed
    // or no data are available
    // return true if parse_chunk() did not stall, otherwise return false
    // (i.e. return the value returned by parse_chunk())
    bool parse() {
        bool ans;
        if (parse_guard) {
            throw std::logic_error("parse_guard!");
        }
        // check if reader has stopped (here to prevent data races)
        if (data_queues->test_end_of_input()) {
            // if also pasing fail cyeheck if data have been all consumed
            if (!(ans = parser.parse_chunk())) {
                // discover why failed: eoi or cannot store chunk?
                if (parser.hold_filled()) {
                    // failed because insertion failed
                    return ans;
                }
                if (parser.hold()) {
                    // chunk partially filled - store it directly
                    !parser.store_partial_chunk();
                    // if fails, should be retried
                }
                if (ans == false) {
                    // no more parsing
                    data_queues->set_end_of_str2num();
                    parse_guard = true;
                }
                // else, failed extraction of new rows and no chunk
                // was being filled
                return ans; // this method should no more be called
            }
        } else {
            ans = parser.parse_chunk();
        }
        return ans;
    }

    // to perform computations - perform computation on exacly one
    // chunk
    // must not be called if compute_guard is true
    // return true if analyze() did not stall, otherwise return false
    // (i.e. return the value returned by analyze())
    bool compute() {
        bool ans;
        if (compute_guard) {
            throw std::logic_error("compute_guard!");
        }
        // similar to parse(): check why analyze() fails
        // check for end of data supplied
        if (data_queues->test_end_of_str2num()) {
            if (!(ans = analyser.analyze())) {
                // extraction failed - no more input will be available
                // mark end of analysis for this worker
                data_queues->set_end_of_analysis();
                compute_guard = true;
                // this method should not be called anymore
                return ans;
            }
        }
        ans = analyser.analyze(); // analyze chunk
        return ans;
    }

    // try to run repeately parse() and then compute()
    // to process some data.
    // must not be called if compute_guard is true
    // Return false if both cannot
    // be called anymore (guards prevent it), otherwise
    // return true
    bool perform_iteration() {
        int i{}, j{};
        // perform some parsing
        for (; i!=parse_repetitions && !parse_guard && parse(); ++i);
        // perform some computation
        for (; j!=compute_repetitions && !compute_guard && compute(); ++j);
#ifdef YIELD
        stalled = i==0 && j==0;
#endif
        return !(parse_guard && compute_guard);
    }

    void spawn_and_run() {
        worker_thread = std::thread([this](){
            while (this->perform_iteration()) {
#ifdef YIELD
                if (stalled) {
                    std::this_thread::yield();
                }
#endif
            }
        });
    }

    void join() {
        worker_thread.join();
    }

    // to obtain final results
    std::valarray<math::statistics::pcc_partial<T>> get_results_and_invalidate() {
        return std::move(analyser.get_results_and_invalidate());
    }
};



#endif
