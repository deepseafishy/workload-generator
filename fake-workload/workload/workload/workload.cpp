#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options.hpp>

#include <cstdint>
#include <iostream>
#include <fakework/memory.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("memory-size", po::value<uint64_t>(), "total memory footprint in MB (default: 128)")
        ("working-set-size", po::value<uint64_t>(), "working set size in MB (default: 64)")
        ("dirty-size", po::value<uint64_t>(), "dirty rate in MB (default: 32)")
        ("period", po::value<uint64_t>(), "the period in milliseconds (default: 1000)")
        ("buckets", po::value<uint64_t>(), "the number of buckets (default: 10)")
        ("threads", po::value<uint64_t>(), "the number of threads for workload generation (default: 4)")
        ("pagesize", po::value<uint64_t>(), "the pagesize (default: 4096)")
        ("cpu-util", po::value<double>(), "the target CPU utilization [0,1] (default: 0.0)")
        ("compressibility", po::value<double>(), "target compressibility of the initial data [0,1] (default: 0.5)")
        ("dirty-compressibility", po::value<double>(), "target compressibility of dirtied data [0,1] (default: 0.5)")
        ("redirtied-ratio", po::value<double>(), "ratio of re-dirtied pages [0,1] (default: 0.0)")
        ("delta-compressibility", po::value<double>(), "ratio of zero words in the re-dirtied data [0,1] (default: 0.5)")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    uint64_t memory_size = 128;
    uint64_t working_set_size = 64;
    uint64_t dirty_size = 32;
    uint64_t period = 1000;
    uint64_t buckets = 10;
    uint64_t threads = 4;
    uint64_t pagesize = 4096;
    double cpu_util = 0.0;
    double compressibility = 0.5;
    double dirty_compressibility = 0.5;
    double redirtied_ratio = 0.0;
    double delta_compressibility = 0.5;

    if (vm.count("memory-size"))
        memory_size = vm["memory-size"].as<uint64_t>();
    if (vm.count("working-set-size"))
        working_set_size = vm["working-set-size"].as<uint64_t>();
    if (vm.count("dirty-size"))
        dirty_size = vm["dirty-size"].as<uint64_t>();
    if (vm.count("period"))
        period = vm["period"].as<uint64_t>();
    if (vm.count("buckets"))
        buckets = vm["buckets"].as<uint64_t>();
    if (vm.count("threads"))
        threads = vm["threads"].as<uint64_t>();
    if (vm.count("pagesize"))
        pagesize = vm["pagesize"].as<uint64_t>();
    if (vm.count("cpu-util"))
        cpu_util = vm["cpu-util"].as<double>();
    if (vm.count("compressibility"))
        compressibility = vm["compressibility"].as<double>();
    if (vm.count("dirty-compressibility"))
        dirty_compressibility = vm["dirty-compressibility"].as<double>();
    if (vm.count("redirtied-ratio"))
        redirtied_ratio = vm["redirtied-ratio"].as<double>();
    if (vm.count("delta-compressibility"))
        delta_compressibility = vm["delta-compressibility"].as<double>();

    std::cout << "memory-size: " << memory_size << std::endl;
    std::cout << "working-set-size: " << working_set_size << std::endl;
    std::cout << "dirty-size: " << dirty_size << std::endl;
    std::cout << "period: " << period << std::endl;
    std::cout << "buckets: " << buckets << std::endl;
    std::cout << "threads: " << threads << std::endl;
    std::cout << "pagesize: " << pagesize << std::endl;
    std::cout << "cpu_util: " << cpu_util << std::endl;
    std::cout << "compressibility: " << compressibility << std::endl;
    std::cout << "dirty-compressibility: " << dirty_compressibility << std::endl;
    std::cout << "redirtied-ratio: " << redirtied_ratio << std::endl;
    std::cout << "delta-compressibility: " << delta_compressibility << std::endl;

    std::unique_ptr<fakework::MemoryWorkload> mem_workload(
            new fakework::MemoryWorkload(
                memory_size,
                working_set_size,
                dirty_size,
                period,
                buckets,
                threads,
                pagesize,
                cpu_util,
                compressibility,
                dirty_compressibility,
                redirtied_ratio,
                delta_compressibility));

    mem_workload->Run();

    return 0;
}
