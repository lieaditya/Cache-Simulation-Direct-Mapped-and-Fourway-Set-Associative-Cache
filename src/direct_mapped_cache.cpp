#include <iostream>
#include <cmath>

#include "../includes/direct_mapped_cache.hpp"
#include "../includes/main_memory_global.hpp"

using namespace std;

DirectMappedCache::DirectMappedCache(unsigned numOfCacheLines, CacheConfig cacheConfig) : numOfCacheLines(numOfCacheLines) {
    cacheLine = new CacheLine[numOfCacheLines];

    // Allocate data[] with a size depending on number of offset bits
    for (unsigned i = 0; i < numOfCacheLines; i++) {
        cacheLine[i].data = new uint8_t[static_cast<uint32_t>(pow(2, cacheConfig.numberOfOffsetBits))]; 
    }
}

DirectMappedCache::~DirectMappedCache() {
    for (unsigned i = 0; i < numOfCacheLines; i++) {
        delete[] cacheLine[i].data;
    }
    delete[] cacheLine;
}

uint32_t DirectMappedCache::read_from_cache(uint32_t address, CacheConfig cacheConfig, Result &result) {
    CacheAddress cacheAddress(address, cacheConfig);
    CacheLine &currentCacheLine = cacheLine[cacheAddress.index];

    // Replace when cold miss or when tag is different, and update number of misses/hits
    bool found = true;
    if (currentCacheLine.isFirstTime || currentCacheLine.tag != cacheAddress.tag) {
        replace(address, currentCacheLine, cacheConfig.numberOfOffsetBits, cacheConfig);
        currentCacheLine.isFirstTime = false;
        result.misses++;
        found = false;
    }
    if (found) {
        result.hits++;
    }

    // Merge 4 bytes of data from 4 consecutive offsets into a single 32-bit-unsigned integer
    uint8_t data1 = currentCacheLine.data[cacheAddress.offset];
    uint8_t data2 = currentCacheLine.data[cacheAddress.offset + 1];
    uint8_t data3 = currentCacheLine.data[cacheAddress.offset + 2];
    uint8_t data4 = currentCacheLine.data[cacheAddress.offset + 3];
    uint32_t dataToRead = merge_data_to_uint32(data1, data2, data3, data4);

    return dataToRead;
}

void DirectMappedCache::write_to_cache(uint32_t address, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result) {
    CacheAddress cacheAddress(address, cacheConfig);
    CacheLine &currentCacheLine = cacheLine[cacheAddress.index];

    // Replace when cold miss or when tag is different, and update number of misses/hits
    bool found = true;
    if (currentCacheLine.isFirstTime || currentCacheLine.tag != cacheAddress.tag) {
        replace(address, currentCacheLine, cacheConfig.numberOfOffsetBits, cacheConfig);
        currentCacheLine.isFirstTime = false;
        result.misses++;
        found = false;
    }
    if (found) {
        result.hits++;
    }

    // Split a 32-bit-unsigned integer into 4 bytes of data with consecutive offsets according to little-endian
    uint8_t byteOfData = static_cast<uint8_t>(dataToWrite & 0xFF);
    currentCacheLine.data[cacheAddress.offset] = byteOfData;
    mainMemory->write_to_ram(address, byteOfData);

    byteOfData = static_cast<uint8_t>((dataToWrite >> 8) & 0xFF);
    currentCacheLine.data[cacheAddress.offset + 1] = byteOfData;
    mainMemory->write_to_ram(address + 1, byteOfData);

    byteOfData = static_cast<uint8_t>((dataToWrite >> 16) & 0xFF);
    currentCacheLine.data[cacheAddress.offset + 2] = byteOfData;
    mainMemory->write_to_ram(address + 2, byteOfData);

    byteOfData = static_cast<uint8_t>((dataToWrite >> 24) & 0xFF);
    currentCacheLine.data[cacheAddress.offset + 3] = byteOfData;
    mainMemory->write_to_ram(address + 3, byteOfData);
}

void DirectMappedCache::replace(uint32_t address, CacheLine &currentCacheLine, uint32_t numberOfOffset, CacheConfig cacheConfig) {
    uint32_t totalOffset = static_cast<uint32_t>(pow(2, numberOfOffset));

    // Fetch a block of data from the main memory
    uint32_t startAddressToFetch = (address / totalOffset) * totalOffset;
    uint32_t lastAddressToFetch = startAddressToFetch + totalOffset - 1;

    for (uint32_t ramAddress = startAddressToFetch, offset = 0; ramAddress <= lastAddressToFetch; ramAddress++, offset++) {
        currentCacheLine.data[offset] = mainMemory->read_from_ram(ramAddress);
    }

    // Update tag
    CacheAddress newAddress(startAddressToFetch, cacheConfig);
    currentCacheLine.tag = newAddress.tag;
}
