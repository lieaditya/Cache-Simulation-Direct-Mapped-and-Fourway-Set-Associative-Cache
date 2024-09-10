#ifndef CACHETEMPLATE_HPP
#define CACHETEMPLATE_HPP

#include "address_structs.hpp"
#include "io_structs.hpp"
#include <cstdint>

class CacheBase {
public:
    virtual ~CacheBase() = default;

    virtual uint32_t read_from_cache(uint32_t address, CacheConfig cacheConfig, Result &result) = 0;
    
    virtual void write_to_cache(uint32_t address, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result) = 0;

    static uint32_t merge_data_to_uint32(uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4);
};

#endif
