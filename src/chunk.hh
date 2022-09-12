#ifndef CHUNK
#define CHUNK

#include <memory>
#include <stdexcept>

// Represent a chunk of data to be processed
// A chunk is a collection of fixed size columns,
// i.e. a fixed size table;
template <typename T>
class chunk {
private:
    const std::size_t _rows{}, _cols{}, _sz{};
    // use only one array to speed up memory accesses
    // and allocations 
    std::unique_ptr<T[]> _data;

    // to add a simple method push_bask
    std::size_t _insert_index{};
    // used to speed up size extimation
    std::size_t _insert_index_row{};
    std::size_t _insert_index_col{};

    // used to eventually support data layout by rows or by columns
    void inc_insert_index() {
#ifdef STORE_BY_ROWS
#pragma message "Organize chunks by rows..."
        ++_insert_index;
        if (++_insert_index_col == _cols) {
            _insert_index_col = 0;
            ++_insert_index_row;
        }
#else
#pragma message "Organize chunks by columns..."
        // next in next column
        if (++_insert_index_col == _cols) {
            // no more column
            _insert_index_col = 0;
            ++_insert_index_row;
            _insert_index = row_col_to_index(_insert_index_row, _insert_index_col);
        } else {
            _insert_index += _rows;
        }
#endif
    }

    // used to eventually support data layout by rows or by columns
    // convert pair (row,column) into linear position
    std::size_t row_col_to_index(std::size_t row, std::size_t col) const {
#ifdef STORE_BY_ROWS
        // data is organized by rows
        return row*_cols + col;
#else
        // data is organized by columns
        return col*_rows + row;
#endif
    }

public:
    chunk(std::size_t rows, std::size_t cols)
    : _rows{rows}, _cols{cols}, _sz{rows*cols}, _data{new T[rows*cols]}
    {}

    chunk& unsafe_push_back(T value) {
        _data[_insert_index] = value;
        inc_insert_index();
        return *this;
    }

    // insertion always happen by row
    chunk& push_back(T value) {
        if (full()) {
            throw std::runtime_error("No more space available in the chunk");
        }
        return unsafe_push_back(value);
    }

    T unsafe_at(std::size_t row, std::size_t col) const {
        auto pos = row_col_to_index(row, col);
        return _data[pos];
    }

    T at(std::size_t row, std::size_t col) const {
        if (row >= _rows || col >= _cols) {
            using namespace std::literals;
            throw std::out_of_range("Invalid range ("s + std::to_string(row) + ","s + std::to_string(col) + ")"s);
        }
        auto pos = row_col_to_index(row, col);
        return _data[pos];
    }

    chunk& clear() {
        // clearing effectively the buffer is unnecessary
        _insert_index = 0;
        _insert_index_col = 0;
        _insert_index_row = 0;
        return *this;
    }

    // is chunk empty?
    bool empty() const {
        return _insert_index == 0;
    }

    // Return the number of the first row after the last completely filled
    // If row number returned is 0 then no rows has been completely filled
    std::size_t rows() const {
        return _insert_index_row;
    }

    // maximum number of rows this chunk can contain
    std::size_t  max_rows() const {
        return _rows;
    }

    // return the number of column this chunk will contains
    std::size_t cols() const {
        return _cols;
    }

    // return the column index the next item will be inserted in
    std::size_t next_row_insertion() const {
        return _insert_index_row;
    }

    // return the row index the next item will be inserted in
    std::size_t next_col_insertion() const {
        return _insert_index_col;
    }

    // no more items can be inserted
    bool full() const {
        return _insert_index_col == 0 && _insert_index_row == _rows;
    }

    // get raw pointer to the beginnin of the giwen column
    T* get_column(std::size_t c) {
#ifdef STORE_BY_ROWS
        return &_data[c];
#else
        return &_data[c*_rows];
#endif
    }

    T* get_row(std::size_t r) {
#ifdef STORE_BY_ROWS
        return &_data[r*_cols];
#else
        return &_data[r];
#endif
    }

    // true in elements in the same column are stored sequentially
    bool stored_by_columns() const {
#ifdef STORE_BY_ROWS
        return false;
#else
        return true;
#endif
    }

    bool stored_by_rows() const {
        return !stored_by_columns();
    }

    // total size of the chunk
    std::size_t size() const {
        return _sz;
    }

    // access directly the underlying vector
    T* data() {
        return _data.get();
    }

    const T* data() const {
        return _data.get();
    }

    T* begin() {
        return data();
    }

    const T* begin() const {
        return data();
    }

    T* end() {
        return data()+size();
    }

    const T* end() const {
        return data()+size();
    }

    // offset (w.r.t. data(), pointer arithmetic)
    // between items in the same row and adjacent
    // column
    std::size_t column_offset() const {
        return stored_by_columns() ? _rows : 1;
    }

    // offset (w.r.t. data(), pointer arithmetic)
    // between items in the same column and adjacent
    // row
    std::size_t row_offset() const {
        return stored_by_columns() ? 1 : _cols;
    }
};

#endif
