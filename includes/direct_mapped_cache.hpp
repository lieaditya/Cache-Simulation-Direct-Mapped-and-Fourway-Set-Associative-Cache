#ifndef DIRECTMAPPEDCACHE_HPP
#define DIRECTMAPPEDCACHE_HPP

#include <cstdint>

#include "address_structs.hpp"
#include "io_structs.hpp"
#include "cache_base.hpp"
#include "main_memory.hpp"

struct CacheLine {
    uint32_t tag;
    uint8_t* data;
    bool isFirstTime = true;
};

class DirectMappedCache : public CacheBase {
private:
    CacheLine* cacheLine;
    unsigned numOfCacheLines;

    void replace(uint32_t address, CacheLine &currentEntry, uint32_t numberOfOffsetBits, CacheConfig cacheConfig);

public:
    DirectMappedCache(unsigned cacheLines, CacheConfig cacheConfig);

    ~DirectMappedCache();

    uint32_t read_from_cache(uint32_t address, CacheConfig cacheConfig, Result &result) override;

    void write_to_cache(uint32_t address, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result) override;
};

#endif
