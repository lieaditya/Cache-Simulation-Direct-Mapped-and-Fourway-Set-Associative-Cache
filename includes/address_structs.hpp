#ifndef ADDRESSSTRUCTS_HPP
#define ADDRESSSTRUCTS_HPP

#include <cstdint>
#include <cmath>

typedef struct CacheConfig {
    int numberOfIndexBits;
    int numberOfTagBits;
    int numberOfOffsetBits;
} CacheConfig;

struct CacheAddress {
    uint32_t index; 
    uint32_t tag;
    uint32_t offset;

    CacheAddress(uint32_t address, CacheConfig cacheConfig) {
        uint32_t offsetFullMask = static_cast<int>(pow(2, cacheConfig.numberOfOffsetBits)) - 1;
        uint32_t indexFullMask= static_cast<int>(pow(2, cacheConfig.numberOfIndexBits)) - 1;

        offset = address & offsetFullMask;
        index = (address >> cacheConfig.numberOfOffsetBits) & indexFullMask;
        tag = (address >> cacheConfig.numberOfOffsetBits) >> cacheConfig.numberOfIndexBits; 
    }
};

#endif
