
#ifndef QUEUES
#define QUEUES

#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"

#include "chunk.hh"

#include <atomic>
#include <vector>
#include <string>

/**
 * The main thread and the worker threads use some
 * queues to share their data. It holds also some
 * synchronization variables used to inform the workers
 * when some queues will no more be used.
 *
 * This class will be used to simplify data passing
 * between main and worker threads.
 * 
 */

template <typename T>
struct queues {

    // a lot of rows are expected to be parsed
    constexpr static std::size_t ROW_QUEUE_SIZE = 100000;
    // chunk are big: the queue is not expected to grow a lot
    constexpr static std::size_t CHUNK_QUEUE_SIZE = 100;

    // specify number of workers that will be used, necessary
    // to estimate end of processing
    const unsigned int worker_count;

    std::size_t rows_per_chunk = 0; // 0 means default

    // queue to be used to transmit 
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>> rowQueue = std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>>(
        new lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>(ROW_QUEUE_SIZE)
    );
    // queue to be used to transmit chunk to be analysed
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunkQueue = std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>>(
        new lockfree_queue::fixed_size_lockfree_queue<chunk<T>>(CHUNK_QUEUE_SIZE)
    );

    // flag to communicate:
    //  end of input,
    std::atomic_bool end_of_input{}; // set only by the main thread
    //  end of str2num convertions, - incremented once per worker
    std::atomic_uint end_of_str2num{};
    //  end of chunk analysing - incremented once per worker
    std::atomic_uint end_of_analysis{};

    // default constructor
    queues(unsigned int workers)
    : worker_count{workers}
    {}

    // prevent moving and copyng
    queues(const queues&) = delete;
    queues(queues&&) = delete;

    // prevent assignment
    queues& operator=(const queues&) = delete;
    queues& operator=(queues&&) = delete;
    
    void set_rows_per_chunk(std::size_t rows_per_chunk) {
        this->rows_per_chunk = rows_per_chunk;
    }

    /* no more input data will be generated */
    void set_end_of_input() {
        end_of_input.store(true);
    }
    bool test_end_of_input() {
        return end_of_input.load();
    }

    // to be called once per worker to mark it will not
    // be able anymore to generate new chunks
    void set_end_of_str2num() {
        end_of_str2num.fetch_add(1);
    }
    // no more chunk will be generated
    bool test_end_of_str2num() const {
        return end_of_str2num.load() == worker_count;
    }

    // called by workers when they will not anymore be able to
    // parse new chunk (because all chunk have been parsed and
    // no more can be fetched)
    void set_end_of_analysis() {
        end_of_analysis.fetch_add(1);
    }
    bool test_end_of_analysis() const {
        return end_of_analysis.load() == worker_count;
    }
};


#endif

