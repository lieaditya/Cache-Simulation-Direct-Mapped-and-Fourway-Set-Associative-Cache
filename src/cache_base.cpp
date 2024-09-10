#include "../includes/cache_base.hpp"

uint32_t CacheBase::merge_data_to_uint32(uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4) {
    // Merge 4 bytes of data with consecutive offsets into a 32-bit-unsigned integer according to little-endian
    uint32_t result = 0;
    result |= static_cast<uint32_t>(data4) << 24;
    result |= static_cast<uint32_t>(data3) << 16;
    result |= static_cast<uint32_t>(data2) << 8; 
    result |= static_cast<uint32_t>(data1); 
    return result;
}