
#ifndef CUDA_NUMERIC_CONSUMER
#define CUDA_NUMERIC_CONSUMER

/**
 * Like class numeric_consumer but relies on GPU
 * to perform computations
 */

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include <vector>
#include <valarray>
#include <algorithm>

#include "chunk.hh"
#include "../modules/CPP-lockfree-queue/fixed_size_lockfree_queue.hh"
#include "../modules/CPP-math-utils/correlation.hh"
#include "../modules/CPP-math-utils/couple.hh"


// CUDA Block Size
#ifndef CUDA_BLOCK_SIZE
#pragma message "CUDA BLOCK SIZE set to 32"
constexpr unsigned int CBS = 32;
#else
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "CUDA BLOCK SIZE set to " STR(CUDA_BLOCK_SIZE)
#undef STR_HELPER
#undef STR
constexpr unsigned int CBS = CUDA_BLOCK_SIZE;
#endif

// Limit number of used CUDA bloks 
#ifdef CUDA_BLOCKS
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "Limit usable CUDA BLOCKs to " STR(CUDA_BLOCKS)
#undef STR_HELPER
#undef STR
#else
#pragma message "No imposed limit on usable CUDA BLOCKs number"
#endif

// Macro can be used anywhere, regardeless of CUDA support for C++ code
#define COUPLE_NUMBER(n) ((n-1)*(n)/2)
// MIN macro
#define MIN(x,y) ((x) < (y) ? (x) : (y))

/****************************************
 ******** AUXILIARY CUDA METHODS ********
 ****************************************/

#ifndef SLOW
#pragma message "Compute cross correlation in a intelligent manner..."

// calculate sum of columns and sum of columns squared
// ideally: one thread per column
template <typename T>
__global__ void colSumKernel(
    // results arguments
    T* totals,              // size: 1xcol column number
    T* squared_totals,      // size: 1xcol column number 
    // imput arguments
    const T* matrix,   // matrix containing the chunk to analyze
    int rows,           // rows in the matrix
    int cols,           // columns in the matrix
    int row_offset,     // offset between same index
                                // elements of two adjacent rows
    int col_offset
)
{
    // to linearize blocks
    auto colId = threadIdx.x + blockDim.x * blockIdx.x;
    const auto poolsz = (gridDim.x ? gridDim.x : 1) * blockDim.x;
    // for each column - hopefully, once per CUDA thread
    for (auto column = matrix + colId*col_offset; colId < cols; colId += poolsz, column += poolsz*col_offset) {
        // accumulators to limit memory access
        T totals_acc{};
        T squared_totals_acc{};
        // for each row in the column
        auto row = column;
        for (int rowId{}; rowId!=rows; ++rowId, row+=row_offset) {
            // use temporary to limit memory access
            const auto val = *row;
            // increment total
            totals_acc += val;
            squared_totals_acc += val*val;
        }
        // store accumuated results
        totals[colId] += totals_acc;
        squared_totals[colId] += squared_totals_acc;
    }
}
#endif


// identify a given column pair
// always: Pair.first < Pair.second
struct Pair {
    int first = 0, second = 0;
};
// to split the work between p threads, we need to find
// the p-th pair
__device__  Pair fast_pair(const int n, const int p)
{
    // closed form formula from: https://stackoverflow.com/questions/21331385/indexing-the-unordered-pairs-of-a-set
    const int to_square = 2 * n - 1;
    const int square = to_square * to_square;
    // floorf
    //  https://docs.nvidia.com/cuda/cuda-math-api/group__CUDA__MATH__SINGLE.html#group__CUDA__MATH__SINGLE
    const int x = floorf((to_square - sqrt(float(square - 8 * p))) / 2);
    const int y = p - (2 * n - x - 3) * x / 2 + 1;
    return Pair{ x, y };
}
// which is the next pair?
__device__ Pair inc(int n, Pair couple) {
    if (++couple.second == n) {
        couple.first += 1;
        couple.second = couple.first + 1;
        return couple;
    }
    return couple;
}


#ifndef SLOW

// helper effectively calculating cross_sum
template <typename T>
__device__ T calculate_cross_sum(
    const T* col1,
    const T* col2,
    int rows,       // rows to consider
    int row_offset  // offset between consecutive elements in the columns
)
{
    // accumulator to limit memory access
    T acc{};
    for (int r{}; r!=rows; ++r) {
        acc += (*col1)*(*col2);
        col1 += row_offset;
        col2 += row_offset;
    }
    return acc;
}


// calculate sum of products of column pairs
// ideally: one thread per column pair
template <typename T>
__global__ void colPairKernel(
    // results argument
    T* covariance_total,    // size: 1x[col*(col-1)/2] pair count
    // imput arguments
    const T* matrix,    // matrix containing the chunk to analyze
    int rows,           // rows in the matrix
    int cols,           // columns in the matrix
    int row_offset,     // offset between same index
                        // elements of two adjacent rows
    int col_offset
)
{
    const auto limit = COUPLE_NUMBER(cols);
    // to linearize blocks
    auto colId = threadIdx.x + blockDim.x * blockIdx.x;
    const auto poolsz = (gridDim.x ? gridDim.x : 1) * blockDim.x;
    // pairs per thread
    auto cpt = limit / poolsz + (limit % poolsz != 0);
    const auto length = cols;

    if (cpt == 0) {
        if (colId < limit) {
            const auto i = colId;
            //auto couple = ::pair(length, i);
            auto couple = ::fast_pair(length, i);
            covariance_total[i] += calculate_cross_sum(
                matrix + couple.first  * col_offset,
                matrix + couple.second * col_offset,
                rows,
                row_offset
            );
        }
    }
    // else at least one columnt per thread
    else {
        // for each pair assigned to this thread
        auto beginning = cpt * colId;
        auto end = MIN(beginning + cpt, limit);
        auto couple = ::fast_pair(length, beginning);
        while (beginning < end) {
            covariance_total[beginning]  += calculate_cross_sum(
                matrix + couple.first  * col_offset,
                matrix + couple.second * col_offset,
                rows,
                row_offset
            );
            // next pair
            couple = ::inc(length, couple);
            ++beginning;
        }
    }
}
#else
#pragma message "Compute cross correlation in a naive manner..."
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// comput partial pcc on two time series
template <typename T>
__device__ void calculate_pcc(
    math::statistics::pcc_partial<T>* partial,
    const T* col1,
    const T* col2,
    int rows,       // rows to consider
    int row_offset  // offset between consecutive elements in the columns
)
{
    T sum_1{};
    T sum_2{};
    T sum_1_squared{};
    T sum_2_squared{};
    T sum_prod{};
    for (int i{}; i != rows; ++i) {
        const auto v_1 = col1[i];
        const auto v_2 = col2[i];
        sum_1 += v_1;
        sum_2 += v_2;
        sum_1_squared += v_1 * v_1;
        sum_2_squared += v_2 * v_2;
        sum_prod += v_1 * v_2;

        col1 += row_offset;
        col2 += row_offset;
    }
    partial->sum_1 += sum_1;
    partial->sum_2 += sum_2;
    partial->sum_1_squared += sum_1_squared;
    partial->sum_2_squared += sum_2_squared;
    partial->sum_prod += sum_prod;
    partial->count += rows;
}

template <typename T>
__global__ void evaluate(
    // results argument
    math::statistics::pcc_partial<T>* ans,  // size: 1x[col*(col-1)/2] pair count
    // imput arguments
    const T* matrix,    // matrix containing the chunk to analyze
    int rows,           // rows in the matrix
    int cols,           // columns in the matrix
    int row_offset,     // offset between same index
                        // elements of two adjacent rows
    int col_offset
)
{
    const auto limit = COUPLE_NUMBER(cols);
    // to linearize blocks
    auto colId = threadIdx.x + blockDim.x * blockIdx.x;
    const auto poolsz = (gridDim.x ? gridDim.x : 1) * blockDim.x;
    // pairs per thread
    auto cpt = limit / poolsz + (limit % poolsz != 0);
    const auto length = cols;

    if (cpt == 0) {
        if (colId < limit) {
            const auto i = colId;
            //auto couple = ::pair(length, i);
            auto couple = ::fast_pair(length, i);
            calculate_pcc(
                ans+i,
                matrix + couple.first  * col_offset,
                matrix + couple.second * col_offset,
                rows,
                row_offset
            );
        }
    }
    // else at least one columnt per thread
    else {
        // for each pair assigned to this thread
        auto beginning = cpt * colId;
        auto end = MIN(beginning + cpt, limit);
        auto couple = ::fast_pair(length, beginning);
        while (beginning < end) {
            calculate_pcc(
                ans+beginning,
                matrix + couple.first  * col_offset,
                matrix + couple.second * col_offset,
                rows,
                row_offset
            );
            // next pair
            couple = ::inc(length, couple);
            ++beginning;
        }
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif


#ifndef SLOW
template <typename T>
__device__ math::statistics::pcc_partial<T> coalesce_single(
    // input parameters
    const T totals1,            const T totals2,
    const T squared_totals1,    const T squared_totals2,
    const T covariance_total,
    const long long rows        // total rows processed
)
{
    math::statistics::pcc_partial<T> ans;
    ans.sum_1 = totals1;
    ans.sum_1_squared = squared_totals1;
    ans.sum_2 = totals2;
    ans.sum_2_squared = squared_totals2;
    ans.sum_prod = covariance_total;
    ans.count = rows;
    return ans;
}

// return an array of pcc_partial to be copied on CPU memory
template <typename T>
__global__ void coalesce(
    const int cols,           // number of columns
    // output parameter
    math::statistics::pcc_partial<T>* ans,
    // input parameters
    const T* totals,
    const T* squared_totals,
    const T* covariance_total,
    const long long rows        // total rows processed
)
{
    // conceptually similar to colPairKernel
    const auto limit = COUPLE_NUMBER(cols);
    // to linearize blocks
    auto colId = threadIdx.x + blockDim.x * blockIdx.x;
    const auto poolsz = (gridDim.x ? gridDim.x : 1) * blockDim.x;
    // pairs per thread
    auto cpt = limit / poolsz + (limit % poolsz != 0);
    const auto length = cols;

    if (cpt == 0) {
        if (colId < limit) {
            const auto i = colId;
            auto couple = ::fast_pair(length, i);

            ans[i] = coalesce_single<T>(
                totals[couple.first], totals[couple.second],
                squared_totals[couple.first], squared_totals[couple.second],
                covariance_total[i],
                rows
            );
        }
    }
    // else at least one columnt per thread
    else {
        // for each pair assigned to this thread
        auto beginning = cpt * colId;
        auto end = MIN(beginning + cpt, limit);
        auto couple = ::fast_pair(length, beginning);
        while (beginning < end) {
            ans[beginning] = coalesce_single(
                totals[couple.first], totals[couple.second],
                squared_totals[couple.first], squared_totals[couple.second],
                covariance_total[beginning],
                rows
            );
            // next pair
            couple = ::inc(length, couple);
            ++beginning;
        }
    }
}
#endif

/***************************************************/
/***************************************************/
/***************************************************/

template <typename T>
class cuda_numeric_consumer {
    // number of columns to be analysed
    const std::size_t col_count;

// INPUT queue: chunk queues to read data to analyze
    // used to support smart memory management
    std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunk_queue_smart_ptr;
    // the queue will be accessed by a row pointer allowing
    // this class to receive both smart and raw pointers 
    lockfree_queue::fixed_size_lockfree_queue<chunk<T>>* chunk_queue_ptr;

    // hold partial results
    struct cuda_results
    {
        /* data */
#ifndef SLOW
        thrust::device_vector<T> totals;
        thrust::device_vector<T> squared_totals;
        thrust::device_vector<T> covariance_total;

        long long rows{}; // total rows processed
#else
        thrust::device_vector<math::statistics::pcc_partial<T>> partials;
#endif

        cuda_results(std::size_t columns)
#ifndef SLOW
        : totals(columns), squared_totals(columns),
          covariance_total(columns*(columns-1)/2)
#else
        : partials(columns*(columns-1)/2)
#endif
        {}
    };
    
    cuda_results results;

    // new chunk to analize
    std::unique_ptr<chunk<T>> new_cnk;

#ifndef NO_PREALLOCATE_BUFFER
    thrust::device_vector<T> matrix;
#endif

    // auxiliary function to perform computations
    void compute() {
#ifdef BLACKHOLE
#pragma message "BLACKHOLE: skip all computation!!!"
#else
        // copy chunk to GPU
#ifdef NO_PREALLOCATE_BUFFER
        thrust::device_vector<T> matrix(new_cnk->begin(), new_cnk->end());
#else
        if (matrix.size() < new_cnk->size()) {
            matrix = thrust::device_vector<T>(new_cnk->begin(), new_cnk->end());
        } else {
            thrust::copy(new_cnk->begin(), new_cnk->end(), matrix.begin());
        }
#endif

        // accumulate;
#ifndef SLOW
        //  per column calculus
        colSumKernel<<<
#ifdef CUDA_BLOCKS
    std::min<unsigned int>(
        CUDA_BLOCKS,
#endif
        (col_count+(CBS-1))/CBS
#ifdef CUDA_BLOCKS
    )
#endif
            ,
        CBS>>>(
            thrust::raw_pointer_cast(&results.totals[0]),
            thrust::raw_pointer_cast(&results.squared_totals[0]),
            thrust::raw_pointer_cast(&matrix[0]),
            new_cnk->rows(),
            new_cnk->cols(),
            new_cnk->row_offset(),
            new_cnk->column_offset()
        );
        //  cross correlation
        colPairKernel<<<
#ifdef CUDA_BLOCKS
    std::min<unsigned int>(
        CUDA_BLOCKS,
#endif
        (COUPLE_NUMBER(col_count)+(CBS-1))/CBS
#ifdef CUDA_BLOCKS
    )
#endif
            ,
        CBS>>>(
            thrust::raw_pointer_cast(&results.covariance_total[0]),
            thrust::raw_pointer_cast(&matrix[0]),
            new_cnk->rows(),
            new_cnk->cols(),
            new_cnk->row_offset(),
            new_cnk->column_offset()
        );
        //  count new rows 
        results.rows += new_cnk->rows();
#else
        // naive approach
        evaluate<<<
#ifdef CUDA_BLOCKS
    std::min<unsigned int>(
        CUDA_BLOCKS,
#endif
        (COUPLE_NUMBER(col_count)+(CBS-1))/CBS
#ifdef CUDA_BLOCKS
    )
#endif
            ,
        CBS
        >>>(
            thrust::raw_pointer_cast(&results.partials[0]),
            thrust::raw_pointer_cast(&matrix[0]),
            new_cnk->rows(),
            new_cnk->cols(),
            new_cnk->row_offset(),
            new_cnk->column_offset()
        );
#endif
#endif  // BLACKHOLE
    }

public:
    cuda_numeric_consumer(std::size_t col_count,
        std::shared_ptr<lockfree_queue::fixed_size_lockfree_queue<chunk<T>>> chunk_queue_smart_ptr
    )
    : col_count{col_count},
      chunk_queue_smart_ptr{std::move(chunk_queue_smart_ptr)},
      chunk_queue_ptr{this->chunk_queue_smart_ptr.get()},
      results(col_count)
    {}

    cuda_numeric_consumer(std::size_t col_count,
        lockfree_queue::fixed_size_lockfree_queue<chunk<T>>* chunk_queue_ptr
    )
    : col_count{col_count},
      chunk_queue_ptr{chunk_queue_ptr},
      results(col_count)
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

    // move data to CPU and invalidate objects
    std::valarray<math::statistics::pcc_partial<T>> get_results_and_invalidate() {
        // preallocate space on host
        std::valarray<math::statistics::pcc_partial<T>> ans(
#ifndef SLOW
            results.covariance_total.size()
#else
            results.partials.size()
#endif
        );
#ifndef BLACKHOLE
#ifndef SLOW
        // preallocate on GPU to coalesce partials
        thrust::device_vector<math::statistics::pcc_partial<T>> gpu_ans(ans.size());
        // coalesce partials
        coalesce<<<
#ifdef CUDA_BLOCKS
    std::min<unsigned int>(
        CUDA_BLOCKS,
#endif
        (COUPLE_NUMBER(col_count)+(CBS-1))/CBS
#ifdef CUDA_BLOCKS
    )
#endif
            ,
        CBS
        >>>(
            col_count,
            thrust::raw_pointer_cast(&gpu_ans[0]),
            thrust::raw_pointer_cast(&results.totals[0]),
            thrust::raw_pointer_cast(&results.squared_totals[0]),
            thrust::raw_pointer_cast(&results.covariance_total[0]),
            results.rows
        );
        cudaDeviceSynchronize();
        // copy to CPU
        thrust::copy(gpu_ans.begin(), gpu_ans.end(), std::begin(ans));
#else
        // direct copy - no computation is required
        thrust::copy(results.partials.begin(), results.partials.end(), std::begin(ans));
#endif
#endif  // BLACKHOLE

        // return to caller
        return std::move(ans);
    }
};

// clear preprocessor "namespace"
#undef COUPLE_NUMBER
#undef MIN


#endif
