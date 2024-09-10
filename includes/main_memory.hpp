#ifndef MAINMEMORY_HPP
#define MAINMEMORY_HPP

#include <cstdint>

class MainMemory {
private:
    uint8_t* data;
    uint32_t memorySize;

public: 
    MainMemory(unsigned cacheAddressLength);
    ~MainMemory();

    uint8_t read_from_ram(uint32_t address);
    
    void write_to_ram(uint32_t address, uint8_t data_to_write);
};

#endif
