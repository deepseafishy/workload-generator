#include <cstdio>
#include <cmath>
#include <cstring>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>

#include <fakework/memory.hpp>

/**
 * fill_data: fill the range with the target compressibility (1 - entropy).
 */
static void fill_data(char *addr, uint64_t size, double entropy)
{
    int32_t unique_chars = (int32_t) pow(256, entropy);
    for (uint64_t i = 0; i < size; ++i)
        addr[i] = std::rand() % unique_chars;
}

/**
 * blend_data: blend the two data sources with the given ratio.
 */
static void blend_data(void *dst, void *src, uint64_t size, double ratio,
                       uint64_t steps)
{
    uint64_t offset = 0;
    uint64_t chunk_size = size / steps;

    for (uint64_t i = 0; i < steps; ++i) {
        offset = chunk_size * i;
        if (((double) std::rand() / (double) RAND_MAX) < ratio)
            memcpy((void *) ((uint64_t) dst + offset),
                   (void *) ((uint64_t) src + offset),
                   chunk_size);
    }
}

/**
 * worker_funtion: the worker thread implementation.
 */
static void worker_function(
        fakework::MemoryBlock *block, uint64_t thread_start_index,
        uint64_t thread_range, uint64_t thread_dirty_pages,
        uint64_t working_set_pages, double redirtied_ratio,
        double delta_compressibility)
{
    uint64_t curr_index = thread_start_index;
    uint64_t dirtied = 0;
    uint64_t thread_end_index = (thread_start_index + thread_range) % working_set_pages;

    while (dirtied < thread_dirty_pages) {
        if (redirtied_ratio > 0.0) {
            block->SetRedirtied(curr_index, delta_compressibility);
            block->SetDirty(curr_index);
        }
        else {
            block->SetDirty(curr_index);
        }

        ++curr_index;
        ++dirtied;

        if (curr_index >= working_set_pages)
            curr_index = curr_index % working_set_pages;
        if (curr_index == thread_end_index)
            curr_index = thread_start_index;
    }
}

namespace fakework
{
    DataPool::DataPool(uint64_t granularity, uint64_t unique_pages)
    {
        this->granularity = granularity;
        this->unique_pages = unique_pages;

        double entropy;
        void *addr;

        int32_t prot = PROT_READ | PROT_WRITE;
        int32_t flags = MAP_PRIVATE | MAP_ANONYMOUS;

        for (uint64_t i = 0; i < this->granularity; ++i) {
            std::vector<void *> arr;
            std::vector<uint64_t> indexes(0, unique_pages);
            entropy = (double) i / (double) this->granularity;
            for (uint64_t j = 0; j < this->unique_pages; ++j) {
                addr = (char *) mmap(NULL, FAKEWORK_PAGE_SIZE, prot, flags, -1, 0);
                if (addr == MAP_FAILED) {
                    std::cout << "MAP_FAILED: " << addr << std::endl;
                }
                fill_data((char *) addr, FAKEWORK_PAGE_SIZE, entropy);
                arr.push_back(addr);
            }
            this->data_pool.push_back(arr);
            this->pool_index.push_back(0UL);
        }
    }

    DataPool::~DataPool(void)
    {
        for (auto arr : this->data_pool)
            for (auto data : arr)
                munmap(data, FAKEWORK_PAGE_SIZE);
    }

    void *DataPool::GetData(double compressibility)
    {
        uint64_t i;
        uint64_t j;
        double entropy = 1.0 - compressibility;

        i = (uint64_t) (entropy * this->granularity);
        i = std::min(i, this->granularity - 1);

        j = this->pool_index[i];
        this->pool_index[i] = ((this->pool_index[i] + 1) % this->unique_pages);

        return this->data_pool[i][j];
    }

    MemoryBlock::MemoryBlock(uint64_t memory_size_mb)
    {
        this->memory_size_mb = memory_size_mb;
        this->memory_bytes = MB_TO_BYTES(memory_size_mb);
        this->max_index = this->memory_bytes / FAKEWORK_PAGE_SIZE;

        int32_t prot = PROT_READ | PROT_WRITE;
        int32_t flags = MAP_PRIVATE | MAP_ANONYMOUS;

        this->addr = mmap(NULL, this->memory_bytes, prot, flags, -1, 0);
        if (this->addr == MAP_FAILED) {
            std::cout << "MAP_FAILED: " << addr << std::endl;
        }

        for (uint64_t i = 0; i < max_index; ++i)
            page_states.push_back(0.0);

        this->data_pool = new fakework::DataPool(20UL, 10UL);
    }

    MemoryBlock::~MemoryBlock(void)
    {
        delete this->data_pool;

        if (this->addr != nullptr) {
            munmap(this->addr, this->memory_bytes);
            std::cout << "Release the memory allocated for this block" << std::endl;
        }
    }

    void *MemoryBlock::GetPage(uint64_t index)
    {
        uint64_t offset = FAKEWORK_PAGE_SIZE * index;
        return (void *) (((uint64_t) this->addr) + offset);
    }

    unsigned long MemoryBlock::GetMaxIndex(void)
    {
        return this->max_index;
    }

    void MemoryBlock::SetDirty(uint64_t index)
    {
        char *addr;

        addr = (char *) GetPage(index);
        addr[0]++;
    }

    void MemoryBlock::SetCompressibility(uint64_t index, double compressibility)
    {
        void *src = this->data_pool->GetData(compressibility);
        void *dst = this->GetPage(index);

        if (this->page_states[index] == compressibility)
            return;

        memcpy(dst, src, FAKEWORK_PAGE_SIZE);
        this->page_states[index] = compressibility;
    }

    void MemoryBlock::SetRedirtied(uint64_t index, double delta_compressibility)
    {
        double ratio = 1.0 - delta_compressibility;
        double compressibility = this->page_states[index];
        void *src = this->data_pool->GetData(compressibility);
        void *dst = this->GetPage(index);

        blend_data(dst, src, FAKEWORK_PAGE_SIZE, ratio, 32UL);
    }

    MemoryWorkload::MemoryWorkload(
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
            double delta_compressibility)
    {
        this->memory_size = memory_size;
        this->working_set_size = working_set_size;
        this->dirty_size = dirty_size;
        this->period = period;
        this->buckets = buckets;
        this->threads = threads;
        this->pagesize = pagesize;
        this->cpu_util = cpu_util;
        this->compressibility = compressibility;
        this->dirty_compressibility = dirty_compressibility;
        this->redirtied_ratio = redirtied_ratio;
        this->delta_compressibility = delta_compressibility;
    }

    void MemoryWorkload::Run(void)
    {
        namespace ch = std::chrono;
        std::unique_ptr<MemoryBlock> block(new MemoryBlock(this->memory_size));

        /*
         * Initialize the memory with the target compressibility.
         */
        for (uint64_t i = 0; i < block->GetMaxIndex(); ++i)
            block->SetCompressibility(i, this->compressibility);

        uint64_t dirty_pages = MB_TO_PAGES(this->dirty_size);
        uint64_t working_set_pages = MB_TO_PAGES(this->working_set_size);

        uint64_t curr_index = 0;
        uint64_t setback = this->redirtied_ratio * dirty_pages;
        /*
         * Use mul_factor to adjust running time of the threads.
         */
        double mul_factor = 1.0;
        uint64_t compute_time = 0;
        uint64_t to_sleep = 0;
        uint64_t time_budget = (1000UL * this->period) / this->buckets;

        std::queue<std::thread *> worker_queue;

        while (true) {
            for (uint64_t i = 0; i < this->buckets; ++i) {
                //uint64_t bucket_range = dirty_pages / this->buckets;
                //uint64_t thread_range = bucket_range / this->threads;

                ch::high_resolution_clock::time_point t1, t2;
                t1 = ch::high_resolution_clock::now();
		 uint64_t bucket_range = dirty_pages / this->buckets;
                uint64_t thread_range = bucket_range / this->threads;

                for (uint64_t j = 0; j < this->threads; ++j) {
                    uint64_t thread_dirty_pages = mul_factor * thread_range;
                    uint64_t thread_start_index = curr_index + thread_range * j;
                    thread_start_index = thread_start_index % working_set_pages;

                    std::thread *worker = new std::thread(
                            worker_function,
                            block.get(),
                            thread_start_index,
                            thread_range,
                            thread_dirty_pages,
                            working_set_pages,
                            this->redirtied_ratio,
                            this->delta_compressibility);
                    worker_queue.push(worker);
                }
                while (!worker_queue.empty()) {
                    std::thread *worker = worker_queue.front();
                    worker->join();
                    worker_queue.pop();
                    delete worker;
                }
                t2 = ch::high_resolution_clock::now();

                compute_time = ch::duration_cast<ch::microseconds>(t2 - t1).count();

                double curr_cpu_util = (double) compute_time / (double) time_budget;
                mul_factor = mul_factor * (this->cpu_util / curr_cpu_util);
                mul_factor = std::max(1.0, mul_factor);

                std::cout << time_budget << " " << compute_time <<  " " << (double) compute_time / time_budget << std::endl;

                if (time_budget > compute_time)
                    to_sleep = time_budget - compute_time;
                else
                    to_sleep = 0UL;

                usleep(to_sleep);

                curr_index = (curr_index + bucket_range) % working_set_pages;
            }

            /*
             * setback the current index to generate re-dirtied pages
             */
            if (setback > curr_index)
                curr_index = working_set_pages - (setback - curr_index);
            else
                curr_index = curr_index - setback;
        }
    }
}
