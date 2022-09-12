
#ifndef NUMERIC_PARSER
#define NUMERIC_PARSER

#include <memory>
#include <vector>
#include <string>
#include <stdexcept>

#include "chunk.hh"
#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"
#include "../modules/CPP-math-utils/convertions.hh"


// This object is used to continously extract rows
// of string to be converted in numeric data 
template <typename T>
class numeric_parser {
public:
    static const std::size_t DEFAULT_ROW_NUMBER = 100;
private:
    // how many rows in each chunk
    std::size_t rows_per_chunk = DEFAULT_ROW_NUMBER;

    // row to parse
    std::unique_ptr<std::vector<std::string>> new_row;
    // chunk to fill
    std::unique_ptr<chunk<T>> curr_cnk;
    // chunk filled? If true try to insert it into output queue
    bool chunk_filled {};

    // number of item in each row
    const std::size_t row_length;

// INPUT queue: string queues to obtain data to parse
    // used to support smart memory management
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>> row_queue_smart_ptr;
    // the queue will be accessed by a row pointer allowing
    // this class to receive both smart and raw pointers 
    lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>* row_queue_ptr;

// OUTPUT queue: chunk queues to store parsed data
    // used to support smart memory management
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunk_queue_smart_ptr;
    // the queue will be accessed by a row pointer allowing
    // this class to receive both smart and raw pointers 
    lockfree_queue::fixed_size_lockfree_queue<chunk<T>>* chunk_queue_ptr;

public:
    numeric_parser(
        std::size_t row_length,
        std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>> row_queue_smart_ptr,
        std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunk_queue_smart_ptr
    )
    : row_length{row_length},
      row_queue_smart_ptr{std::move(row_queue_smart_ptr)},
      row_queue_ptr{this->row_queue_smart_ptr.get()},
      chunk_queue_smart_ptr{std::move(chunk_queue_smart_ptr)},
      chunk_queue_ptr{this->chunk_queue_smart_ptr.get()}
    {}

    numeric_parser(
        std::size_t row_length,
        lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>* row_queue_ptr,
        lockfree_queue::fixed_size_lockfree_queue<chunk<T>>* chunk_queue_ptr
    )
    : row_length{row_length},
      row_queue_ptr{row_queue_ptr},
      chunk_queue_ptr{chunk_queue_ptr}
    {}

    void set_rows_per_chunk(std::size_t rows_per_chunk) {
        // cannot change chunk size after parsing has been started
        if (curr_cnk) {
            throw std::logic_error("Cannot change chunk size (rows) after parsing has been started.");
        }
        this->rows_per_chunk = rows_per_chunk;
    }

    // read rows from the INPUT queue to get strings to parse
    // and partially build the next chunk
    // return true if a chunk as been successfully
    // stored, false otherwise
    bool parse_chunk() {
        // was previously filled chunk transferred into OUTPUT queue
        if (chunk_filled) {
            // try to store filled chunk
            if (chunk_queue_ptr->offer(curr_cnk)) {
                chunk_filled = false;
                return true;
            }
            // attempt failed, will retry later
            return false;
        } else {
            // pick new rows until chunk has been filled
            for (;;) {
                // get new input row
                if (!row_queue_ptr->poll(new_row)) {
                    return false;
                }
                if (!curr_cnk) {
                    curr_cnk = std::make_unique<chunk<T>>(chunk<T>(rows_per_chunk, row_length));
                }
                for (const auto& str : *new_row) {
                    curr_cnk->unsafe_push_back(math::convertions::ston<T>(str));
                }
                new_row.release();
                // check if chunk is full
                if (curr_cnk->full()) {
                    // try to insert
                    if (!chunk_queue_ptr->offer(curr_cnk)) {
                        // failed, defer insertion
                        chunk_filled = true;
                        return false;
                    } else {
                        // success!
                        return true;
                    }
                }
                // go to new row
            }
        }
    }

    // continously parse chunk until failure
    void parse_many() {
        while (parse_chunk());
    }

    // try to load partially filled chunk (if available, otherwise
    // immediately return true)
    bool store_partial_chunk() {
        if (!curr_cnk || curr_cnk->empty()) {
            return true;
        } else {
            // mark current chunk as filled (ready to be stored)
            chunk_filled = true;
            // try to store chunk
            if (chunk_queue_ptr->offer(curr_cnk)) {
                chunk_filled = false;
                return true;
            }
            // attempt failed, will retry later
            return false;
        }
    }

    // return true if a chunk is hold now
    bool hold() const {
        return curr_cnk.get() != nullptr;
    }

    // return true if it is waiting to store a new chunk
    bool hold_filled() const {
        return chunk_filled;
    }
};


#endif
