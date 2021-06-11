#ifndef FAKEWORK_MEMORY_HPP
#define FAKEWORK_MEMORY_HPP

#include <cstdint>
#include <vector>
#include <random>

#define FAKEWORK_PAGE_SIZE  4096UL

#define MB_TO_BYTES(a) \
  (((uint64_t) a) * (1UL << 20UL))

#define MB_TO_PAGES(a) \
  ((((uint64_t) a) * (1UL << 20UL)) / FAKEWORK_PAGE_SIZE)

namespace fakework
{
  class DataPool {
    private:
        uint64_t granularity;
        uint64_t unique_pages;

        std::vector<std::vector<void *>> data_pool;
        std::vector<uint64_t> pool_index;
    public:
      DataPool(uint64_t granularity, uint64_t unique_pages);
      ~DataPool(void);
      void *GetData(double compressibility);
  };

  class MemoryBlock {
    private:
      uint64_t memory_size_mb;
      uint64_t memory_bytes;
      uint64_t max_index;
      void *addr;
      fakework::DataPool *data_pool;

      /**
       * page_states stores the latest compressibility of the pages.
       */
      std::vector<double> page_states;

      /**
       * Get address of the page at the index.
       */
      void *GetPage(uint64_t index);

    public:

      MemoryBlock(uint64_t memory_size_mb);
      ~MemoryBlock();

      /**
       * Get the maximum index of the memory block.
       */
      uint64_t GetMaxIndex(void);

      /**
       * Make a dirty page by adding 1 to the first byte of the page.
       */
      void SetDirty(uint64_t index);

      /**
       * Set compressibility of the page at index.
       */
      void SetCompressibility(uint64_t index, double compressibility);

      /**
       * Modify subsets of the page with random bytes to adjust the number of
       * zero words in the XOR output.
       */
      void SetRedirtied(uint64_t index, double zero_ratio);
  };

  class MemoryWorkload{
    private:
      uint64_t memory_size;
      uint64_t working_set_size;
      uint64_t dirty_size;
      uint64_t period;
      uint64_t buckets;
      uint64_t threads;
      uint64_t pagesize;

      double cpu_util;
      double compressibility;
      double dirty_compressibility;
      double redirtied_ratio;
      double delta_compressibility;

    public:
      MemoryWorkload(
          uint64_t memory_size,
          uint64_t working_set_size,
          uint64_t dirty_size,
          uint64_t period,
          uint64_t buckets,
          uint64_t threads,
          uint64_t pagesize,
          double cpu_util,
          double compressibility,
          double dirty_compressibility,
          double redirtied_ratio,
          double delta_compressibility);

      void Run(void);
  };
}

#endif
