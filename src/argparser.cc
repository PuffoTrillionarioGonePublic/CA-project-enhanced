#include "argparser.hh"

#include <getopt.h>
#include <thread>

[[noreturn]] void help(const char * const exe) {
    std::cerr << "Usage:\n";
    std::cerr << '\t' << exe << " [--worksers NUM, default $(nproc)-1] [--rows NUM] input-file";
    std::cerr << "Usage:\n";
    
    exit(EXIT_FAILURE);
}

parsed_arguments parse(const int argc, const char *argv[]) {
    // from <getopt.h>
    extern char *optarg;
    extern int optind, opterr, optopt;

    parsed_arguments ans {};
    ans.worker_count = std::thread::hardware_concurrency()-1;
    // used to semplify use of getopt_long 
    enum class options {
        workers
    };
    option longopts[] = {
        // to specify how many worker thread to be used
        { "workers", required_argument, nullptr, 0 },
        // to specify how many rows to put in each chunk
        { "rows", required_argument, nullptr, 0 },
        // last element of the array has to be filled with 0s
        {}
    };
    int longindex {};

    opterr = 0; // getopt* will not show error messages
    for (int i{}; i!=argc; ++i) {
        auto s = std::string(argv[i]);
        if (s == "-h" || s == "--help") {
            throw parsing_exception("");
        }
    }
    while (true) {
        using namespace std::literals;
        longindex = -1;
        int c = getopt_long(argc, (char*const*)argv, "", longopts, &longindex);
        if (c == -1) break;
        if (c == '?') throw parsing_exception("Unknow option found"s);
        if (c == 0) {
            switch (longindex)
            {
            case 0: // handle --workers
                if (!optarg) {
                    throw parsing_exception("Missing value for --worker"s);
                }
                try
                {
                    ans.worker_count = std::stoul(optarg);
                    if (std::to_string(ans.worker_count) != optarg) {
                        throw std::exception();
                    }
                }
                catch(const std::exception&)
                {
                    throw parsing_exception("Invalid value for --worker: "s + optarg);
                }
                break;
            case 1: // handle --rows
                if (!optarg) {
                    throw parsing_exception("Missing value for --rows"s);
                }
                try
                {
                    ans.row_count = std::stoul(optarg);
                    if (std::to_string(ans.row_count) != optarg) {
                        throw std::exception();
                    }
                }
                catch(const std::exception&)
                {
                    throw parsing_exception("Invalid value for --rows: "s + optarg);
                }
                break;
            default:
                throw parsing_exception("Unknow long option found: "s + longopts[longindex].name);
                break;
            }
        } else {
            throw parsing_exception("Unknow option found"s);
        }
    }
    
    // take non option arguments, i.e. input file name:
    if (optind + 1 == argc) {
        ans.input_file = argv[optind];
    } else {
        using namespace std::literals;
        throw parsing_exception("Missing input file"s);
    }

    return ans;
}

