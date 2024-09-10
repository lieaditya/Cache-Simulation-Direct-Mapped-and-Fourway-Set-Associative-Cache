#include "../includes/cache_module.hpp"

MainMemory* mainMemory = new MainMemory(CACHE_ADDRESS_LENGTH);

CACHE_MODULE::CACHE_MODULE(sc_module_name name, int cycles, int directMapped, unsigned cacheLines, unsigned cacheLineSize,
                unsigned cacheLatency, unsigned memoryLatency, int numRequests) : sc_module(name) {
        
    this->cycles = cycles;
    this->directMapped = directMapped;
    this->cacheLines = cacheLines;
    this->cacheLineSize = cacheLineSize;
    this->cacheLatency = cacheLatency;
    this->memoryLatency = memoryLatency;
    this->numRequests = numRequests; 

    waitForCacheLatency.write(0);
    waitForMemoryLatency.write(0);
    requestsExceedCycles.write(0);

    resultTemp.cycles = 0;  
    resultTemp.hits = 0;
    resultTemp.misses = 0;
    resultTemp.primitiveGateCount = 0;

    // Determine number of index, offset, tag
    cacheConfig.numberOfIndexBits = ceil(log2((directMapped == 1) ? cacheLines : cacheLines / 4));
    cacheConfig.numberOfOffsetBits = ceil(log2(cacheLineSize));
    cacheConfig.numberOfTagBits = CACHE_ADDRESS_LENGTH - cacheConfig.numberOfIndexBits - cacheConfig.numberOfOffsetBits;

    // Polymorphic implementation of cache
    if (directMapped == 0) {
        cache = new FourWayLRUCache(cacheConfig);
    } else {
        cache = new DirectMappedCache(cacheLines, cacheConfig);
    }

    // primitiveGateCount
    uint32_t oneBitStorageGates = 4;
    uint32_t allBitsCacheStorageGates = (8 * oneBitStorageGates) * (cacheLines * cacheLineSize);
    uint32_t controlLogicGates = 5 * cacheLines; 
    uint32_t tagComparisonGates = (2 * cacheConfig.numberOfTagBits) * cacheLines;

    totalGates = allBitsCacheStorageGates + tagComparisonGates + controlLogicGates;

    if (!directMapped) {
        uint32_t twoBitCounterGates = (2 * oneBitStorageGates) * cacheLines;
        uint32_t comparatorForCounterGates = twoBitCounterGates * 2; // comparator = 2 gates
        uint32_t updateLogicGates = twoBitCounterGates * 7; // 2-bit adder = HA (2) + VA (5) = 7 gates
        uint32_t LRUGates = twoBitCounterGates + comparatorForCounterGates + updateLogicGates;
        totalGates += LRUGates;
    }

    SC_THREAD(update);
    sensitive << clk.pos();
}

void CACHE_MODULE::update() {
    // Update primitiveGateCount based on calculated totalGates in constructor
    wait(SC_ZERO_TIME);
    resultPrimitiveGateCount.write(totalGates);
    wait(SC_ZERO_TIME);
    resultTemp.primitiveGateCount = resultPrimitiveGateCount.read();

    unsigned cacheLatencyTemp = cacheLatency;
    unsigned memoryLatencyTemp = memoryLatency;
    
    for (int i = 0; i < cycles; i++){
        // If not all requests could be processed within the given cycles, cycles should have the value SIZE_MAX
        if (requestsExceedCycles.read()) {
            resultCycles.write(SIZE_MAX - 1);
            resultTemp.cycles = SIZE_MAX - 1;
        }

        // Wait for cacheLatency cycles to complete before accessing the cache, and reset once completed
        if (cacheLatency > 0 && !waitForMemoryLatency.read()) {
            cacheLatency--;
            resultCycles.write(resultCycles.read() + 1);
            waitForCacheLatency.write(1);
            wait(SC_ZERO_TIME);
            wait();
            continue;
        } else if (cacheLatency == 0) {
            cacheLatency = cacheLatencyTemp;
            waitForCacheLatency.write(0);
            wait(SC_ZERO_TIME);
        }

        // Besides cacheLatency, wait for memoryLatency as well before proceeding
        uint32_t dataToWriteTemp;
        size_t currentMisses = resultMisses;
        wait(SC_ZERO_TIME);
        if (!waitForMemoryLatency.read()) {
            if (requestWE) {
                cache->write_to_cache(requestAddr, cacheConfig, requestData, resultTemp);
            } else {
                dataToWriteTemp = cache->read_from_cache(requestAddr, cacheConfig, resultTemp);
            }
            resultHits.write(resultTemp.hits);
            resultMisses.write(resultTemp.misses);
            wait(SC_ZERO_TIME);
        }

        // Detect cache miss
        if (resultTemp.misses > currentMisses) {
            waitForMemoryLatency.write(1);
            wait(SC_ZERO_TIME);
        }

        // Reset memory latency once completed
        if (memoryLatency > 0 && waitForMemoryLatency.read()) {
            memoryLatency--;
            resultCycles.write(resultCycles.read() + 1);
            wait(SC_ZERO_TIME);
            wait();
            continue;
        }
        if (memoryLatency == 0) {
            memoryLatency = memoryLatencyTemp;
            waitForMemoryLatency.write(0);
            wait(SC_ZERO_TIME);
        }

        // Data-to-read should only be ready after cacheLatency (+ memoryLatency)
        if (!requestWE && !waitForMemoryLatency.read()) {
            data.write(dataToWriteTemp);
            wait(SC_ZERO_TIME);
        }

        // Update signals in result
        resultCycles.write(resultCycles.read() + 1);  
        wait(SC_ZERO_TIME);  
        resultTemp.cycles = resultCycles.read();
        wait(); 
    }
}
