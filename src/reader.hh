
#ifndef READER
#define READER

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"
#include "../modules/CPP-csv-parser/csv.hh"


/**
 * @brief The reader is the component that will parse the input
 * and put the parsed strings in a queue to be extracted by the
 * converter to numeric types.
 */
class reader {
public:
    class end_of_inputs : public std::exception {};
private:
    // name of the input file to be read
    std::string input_file;

    csv::reader csv_in;

    // used to support smart memory management
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>> row_queue_smart_ptr;
    // the queue will be accessed by a row pointer allowing
    // this class to receive both smart and raw pointers 
    lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>* row_queue_ptr;
    // if data cannot be are maintained here
    std::unique_ptr<std::vector<std::string>> holder;

//    reader(std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>> row_queue_smart_ptr)
//    : row_queue_smart_ptr{std::move(row_queue_smart_ptr)}, row_queue_ptr{this->row_queue_smart_ptr.get()}
//    {}
//
//    reader(lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>* row_queue_ptr)
//    : row_queue_ptr{row_queue_ptr}
//    {}
public:

    reader(std::string filename, std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>> row_queue_smart_ptr)
    : input_file{std::move(filename)}, csv_in(std::unique_ptr<std::istream>(new std::ifstream(input_file))), row_queue_smart_ptr{std::move(row_queue_smart_ptr)}, row_queue_ptr{this->row_queue_smart_ptr.get()}
    {}

    reader(std::string filename, lockfree_queue::fixed_size_lockfree_queue<std::vector<std::string>>* row_queue_ptr)
    : input_file{std::move(filename)}, csv_in(std::unique_ptr<std::istream>(new std::ifstream(input_file))), row_queue_ptr{row_queue_ptr}
    {}

    // consume a single row of the input and put
    // parsed data are enqueued in the queue
    // return true if data have been successfully inserted
    // return false if there are no more data to read
    bool consume_row()
    try
    {
        if (holder) {
            return row_queue_ptr->offer(holder);    
        }
        auto line = csv_in.getline().access_and_invalidate();
        holder = std::make_unique<std::vector<std::string>>(line);
        return row_queue_ptr->offer(holder);
    }
    catch (csv::eof&)
    {
        throw end_of_inputs{};
    }

    // consume rows until the queue is full and the insertion fails
    // return true if end of input is reached and false if an
    // enqueeing is failed
    bool consume_many()
    try
    {
        while (consume_row());
        return false;
    }
    catch (end_of_inputs&)
    {
        return true;
    }

    // return the number of column in the given dataset
    std::size_t column_count() {
        return csv_in.column_count();
    }
};


#endif
