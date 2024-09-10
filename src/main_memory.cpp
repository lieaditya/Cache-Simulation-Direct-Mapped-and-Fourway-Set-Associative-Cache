#include <iostream>
#include <cmath>

#include "../includes/main_memory.hpp"

using namespace std;

MainMemory::MainMemory(unsigned cacheAddressLength) {
    memorySize = static_cast<uint32_t>(pow(2, cacheAddressLength));
    data = new uint8_t[memorySize];
    
}

MainMemory::~MainMemory() {
    delete[] data;
}

uint8_t MainMemory::read_from_ram(uint32_t address) {
    if (address >= memorySize) {
        cerr << "Error: Invalid memory address " << address << endl;
        return -1;
    }

    return data[address];
}

void MainMemory::write_to_ram(uint32_t address, uint8_t dataToWrite) {
    if (address >= memorySize) {
        cerr << "Error: Invalid memory address " << address << endl;
    }
    data[address] = dataToWrite;
}   
