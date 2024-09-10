#ifndef CACHEMODULE_HPP
#define CACHEMODULE_HPP

#include <systemc>
#include <iostream>
#include <cstdint>

#include "direct_mapped_cache.hpp"
#include "four_way_lru_cache.hpp"
#include "main_memory.hpp"
#include "address_structs.hpp"
#include "io_structs.hpp"
#include "main_memory_global.hpp"
#include "cache_base.hpp"
#define CACHE_ADDRESS_LENGTH 16

using namespace std;
using namespace sc_core;

extern "C" Result run_simulation(int cycles, bool directMapped,  unsigned cacheLines, unsigned cacheLineSize, 
                        unsigned cacheLatency, int memoryLatency, size_t numRequests, 
                        Request requests[], const char* tracefile);

extern MainMemory* main_memory;

SC_MODULE(CACHE_MODULE) {
    sc_in<bool> clk;
    sc_in<int> requestWE;
    sc_in<uint32_t> requestAddr;
    sc_in<uint32_t> requestData;

    sc_out<size_t> resultCycles;
    sc_out<size_t> resultHits;
    sc_out<size_t> resultMisses;
    sc_out<size_t> resultPrimitiveGateCount;

    sc_signal<int> data;
    sc_signal<bool> waitForCacheLatency;
    sc_signal<bool> waitForMemoryLatency;
    sc_signal<bool> requestsExceedCycles;

    CacheBase* cache; 
    CacheConfig cacheConfig;
    Result resultTemp;
    int cycles;
    int directMapped;
    unsigned cacheLines;
    unsigned cacheLineSize;
    unsigned cacheLatency;
    unsigned memoryLatency;
    int numRequests;
    uint32_t totalGates;

    SC_CTOR(CACHE_MODULE);
    CACHE_MODULE(sc_module_name name, int cycles, int directMapped, unsigned cacheLines, unsigned cacheLineSize,
                unsigned cacheLatency, unsigned memoryLatency, int numRequests);

    void update();  
};

#endif
