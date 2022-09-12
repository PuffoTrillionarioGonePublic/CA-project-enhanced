
#ifndef NUMERIC_CONSUMER
#define NUMERIC_CONSUMER

#include <vector>
#include <valarray>

#include "chunk.hh"
#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"
#include "../modules/CPP-math-utils/correlation.hh"
#include "../modules/CPP-math-utils/couple.hh"

/**
 * @brief This class is intended to receive chunks and process them to
 * calculate the cross correlation of every column pairs. It relies on
 * CPU only.
 * 
 * @tparam T numeric type to be used 
 */
template <typename T>
class numeric_consumer {
    // number of columns to be analysed
    const std::size_t col_count;

// INPUT queue: chunk queues to read data to analyze
    // used to support smart memory management
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunk_queue_smart_ptr;
    // the queue will be accessed by a row pointer allowing
    // this class to receive both smart and raw pointers 
    lockfree_queue::fixed_size_lockfree_queue<chunk<T>>* chunk_queue_ptr;

#ifdef SLOW
    // vector containing results, when it is returned, the object is invalidated
    std::valarray<math::statistics::pcc_partial<T>> partials;
#else
    // to perform computation in an efficient way;
    math::statistics::multicolumn_pcc_accumulator<T> accumulator;
#endif

    // new chunk to analize
    std::unique_ptr<chunk<T>> new_cnk;

    // auxiliary function to perform computations
    void compute() {
#ifdef BLACKHOLE
#pragma message "BLACKHOLE: skip all computation!!!"
#else
#ifdef SLOW
        // filled row in the chunk
        const auto chunk_rows = new_cnk->rows();
        const auto col_count_minus_1 = col_count - 1;
        // index of the item in the vector being update
        std::size_t couple_idx {};
        for (std::size_t c1{}; c1 != col_count_minus_1; ++c1) {
            const T* const col_1 = new_cnk->get_column(c1);
            for (std::size_t c2{c1+1}; c2 != col_count; ++c2) {
                const T* const col_2 = new_cnk->get_column(c2);
                partials[couple_idx++] += math::statistics::pearson_correlation_coefficient(col_1, col_2, chunk_rows);
            }
        }
#else
        accumulator.accumulate(
            new_cnk->data(),
            new_cnk->rows(),
            new_cnk->cols(),
            new_cnk->row_offset(),
            new_cnk->column_offset()
        );
#endif
#endif  // BLACKHOLE
    }

public:
    numeric_consumer(std::size_t col_count,
        std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunk_queue_smart_ptr
    )
    : col_count{col_count},
      chunk_queue_smart_ptr{std::move(chunk_queue_smart_ptr)},
      chunk_queue_ptr{this->chunk_queue_smart_ptr.get()},
#ifdef SLOW
      partials(col_count*(col_count-1)/2)
#else
      accumulator(col_count)
#endif
    {}

    numeric_consumer(std::size_t col_count,
        lockfree_queue::fixed_size_lockfree_queue<chunk<T>>* chunk_queue_ptr
    )
    : col_count{col_count},
      chunk_queue_ptr{chunk_queue_ptr},
#ifdef SLOW
      partials(col_count*(col_count-1)/2)
#else
      accumulator(col_count)
#endif
    {}

    // try to extract a single chunk and process it
    // return true if a chunk is found, false otherwise
    bool analyze() {
        // try to extract a new chunk to analize
        if (!chunk_queue_ptr->poll(new_cnk)) {
            return false;
        }
        compute();
        new_cnk.release();
        return true;
    }

    // continuosly prelevate chunks and parse them
    void analyze_many() {
        while (analyze());
    }

    // return results and invalidate objects
#ifdef SLOW
    std::valarray<math::statistics::pcc_partial<T>>&& get_results_and_invalidate() {
        return std::move(partials);
#else
    std::valarray<math::statistics::pcc_partial<T>> get_results_and_invalidate() {
        return accumulator.to_pcc_partial_valarray();
#endif
    }
};

#endif
