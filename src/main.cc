
#include <iostream>
#include <memory>
#include <algorithm>
#include <thread>

#include "argparser.hh"
#include "queues.hh"
#include "worker.hh"
#include "reader.hh"


#ifdef GPU
#include "cuda_numeric_consumer.hh"
#endif

/**
 * This will be the structure of the program:
 *  the main thread will spawn some other workers thread
 */


int main(int argc, char const *argv[])
try
{
#ifdef FLOAT
#pragma message "Perform computations using float..."
    using data_type = float;
#else
#pragma message "Perform computations using double..."
    using data_type = double;
#endif
#ifdef GPU
#pragma message "Compiling code to use NVIDIA GPU..."
    using worker_type = worker<data_type, cuda_numeric_consumer>;
#else
#pragma message "Compiling code to use CPU only..."
    using worker_type = worker<data_type>;
#endif

    // nWorkers-1 are on separated thread
    // the last is on the main thread

    auto parsed = parse(argc, argv);

    const unsigned int nWorkers = 1 + parsed.worker_count;

    // generate queues
    std::shared_ptr<queues<data_type>> data_queues(
        new queues<data_type>(nWorkers)
    );

    // if specified in argv, set rows per chunk
    if (parsed.row_count) {
        data_queues->set_rows_per_chunk(parsed.row_count);
    }

    // generate reader - necessary to get column count
    reader r(parsed.input_file, data_queues->rowQueue);

    // get column count from the input file
    const auto column_count = r.column_count();
    //const auto result_count = worker<data_type>::result_size_from_column_count(column_count);
    // will hold result type

    // spawn workers
    std::vector<std::unique_ptr<worker_type>> workers; workers.reserve(nWorkers);
    for (std::size_t _{1}; _!=nWorkers; ++_) {
        workers.emplace_back(new worker_type(column_count, data_queues));
        workers.back()->spawn_and_run();
    }

    // generate worker executing while IO stalls
    worker_type main_worker(column_count, data_queues);

    // read input untill it ends
    while (!r.consume_many()) {
        // IO stalls, perform some computations on
        // main thread
        main_worker.perform_iteration();
    }
    // END OF INPUT REACHED!
    data_queues->set_end_of_input();
    // finish computations on main thread
    while (main_worker.perform_iteration());
    
    // accumulate results:
    // initially from main thread
    //std::valarray<math::statistics::pcc_partial<data_type>> results(result_count);
    auto results = main_worker.get_results_and_invalidate();

    // sum results from other workers
    for (auto& w : workers) {
        w->join();
        results += w->get_results_and_invalidate();
    }

    auto resSize = results.size();
    std::valarray<data_type> analysed(resSize);
    for (std::size_t _{}; _!=resSize; ++_) {
        analysed[_] = results[_].compute();
    }

    std::size_t couple_idx {};
    for (std::size_t c1{}; c1 != column_count; ++c1) {
        for (std::size_t c2{c1+1}; c2 != column_count; ++c2) {
            std::cout << '(' << c1 << ',' << c2 << ") " << analysed[couple_idx++] << '\n';
        }
    }

    // just for catching bugs
    if (couple_idx != resSize) {
        throw std::logic_error("couple_idx != resSize");
    }

    return 0;
}
catch (const parsing_exception& pe)
{
    if (std::string(pe.what()).size()) {
        std::cerr << "Error occurred while parsing inputs: " << pe.what() << '\n';
    }
    help(argv[0]);
}
catch (const std::exception& e)
{
    std::cerr << "Unexpected exception: " << e.what() << '\n';
}

