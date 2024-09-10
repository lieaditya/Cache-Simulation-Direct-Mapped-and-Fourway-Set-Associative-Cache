#include <systemc>
#include <iostream>

#include "../includes/io_structs.hpp"
#include "../includes/cache_module.hpp"
#define MATRIX_SIZE 4

using namespace std;
using namespace sc_core;

uint32_t A[MATRIX_SIZE][MATRIX_SIZE] =
    {
        {3, 7, 4 ,12},
        {6, 18, 8, 1},
        {5, 23, 3 ,41},
        {29, 17, 5, 1}
    };

uint32_t B[MATRIX_SIZE][MATRIX_SIZE] = 
    {
        {19, 13, 49, 22},
        {4, 21, 37, 34},
        {50, 0, 8, 14},
        {26, 7, 13, 0}
    };

uint32_t C[MATRIX_SIZE][MATRIX_SIZE] = 
    {
        {597, 270, 594, 360},
        {612, 463, 1037, 856},
        {1403, 835, 1653, 934},
        {895, 741, 2103, 1286}
    };

uint32_t addressA[MATRIX_SIZE][MATRIX_SIZE] =
    {
        {0x0, 0x4, 0x8, 0xC},
        {0x10, 0x14, 0x18, 0x1C},
        {0x20, 0x24, 0x28, 0x2C},
        {0x30, 0x34, 0x38, 0x3C}
    };

uint32_t addressB[MATRIX_SIZE][MATRIX_SIZE] =
    {
        {0x74, 0x78, 0x7C, 0x80},
        {0x84, 0x88, 0x8C, 0x90},
        {0x94, 0x98, 0x9C, 0xA0},
        {0xA4, 0xA8, 0xAC, 0xB0}
    };

uint32_t addressC[MATRIX_SIZE][MATRIX_SIZE] =
    {
        {0xC0, 0xC4, 0xC8, 0xCC},
        {0xD0, 0xD4, 0xD8, 0xDC},
        {0xE0, 0xE4, 0xE8, 0xEC},
        {0xF0, 0xF4, 0xF8, 0xFC}
    };

int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR" << std::endl;
    return 1;
}

Result run_simulation(int cycles, bool directMapped,  unsigned cacheLines, unsigned cacheLineSize, unsigned cacheLatency,
                             int memoryLatency, size_t numRequests, Request requests[], const char* tracefile) {

    sc_clock clk("clk", 1, SC_SEC);
    sc_signal<uint32_t> requestAddr;
    sc_signal<uint32_t> requestData;
    sc_signal<int> requestWE;
    sc_signal<size_t> resultCycles;
    sc_signal<size_t> resultHits;
    sc_signal<size_t> resultMisses;
    sc_signal<size_t> resultPrimitiveGateCount;

    Result result;

    // Ensure tracefile is initialized successfully
    sc_trace_file* simulationTracefile;
    bool simulationTracefileCreated = false;
    if (strcmp(tracefile, "") != 0) {
        string tracefilePath = string("../out/") + string(tracefile);
        simulationTracefile = sc_create_vcd_trace_file(tracefilePath.c_str());
        simulationTracefileCreated = true;
    }

    if (simulationTracefileCreated) {
        if (simulationTracefile == NULL) {
            fprintf(stderr, "simulationTracefile not opened.\n");
            exit(EXIT_FAILURE);
        }
        sc_trace(simulationTracefile, clk, "Clock");
        sc_trace(simulationTracefile, requestAddr, "Request Address");
        sc_trace(simulationTracefile, requestData, "Request Data");
        sc_trace(simulationTracefile, requestWE, "Request WE");
        sc_trace(simulationTracefile, resultCycles, "Result Cycles");
        sc_trace(simulationTracefile, resultMisses, "Result Misses");
        sc_trace(simulationTracefile, resultHits, "Result Hits");
        sc_trace(simulationTracefile, resultPrimitiveGateCount, "Result Primitive Gate Count");
    }

    CACHE_MODULE cache ("cache", cycles, directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency, numRequests);
    
    // Connnect ports to signals
    cache.clk(clk);
    cache.requestAddr(requestAddr);
    cache.requestWE(requestWE);
    cache.requestData(requestData);
    cache.resultCycles(resultCycles);
    cache.resultHits(resultHits);
    cache.resultMisses(resultMisses);
    cache.resultPrimitiveGateCount(resultPrimitiveGateCount);

    // Simulation: Matrix multiplication, only use for matrix_multiplication.csv
    uint32_t entryMatrixA = 0;
    uint32_t entryMatrixB = 0;
    uint32_t entryMatrixC = 0;
    uint32_t mulResultTemp;

    bool readMatrixA = true;
    bool readMatrixB = false;
    bool readMatrixC = false;
    bool isInitializationFinished = false;

    uint32_t a_j = 0;
    uint32_t b_i = 0;
    uint32_t c_i = 0;
    uint32_t c_j = 0;
    uint32_t c = 0;

    size_t requestIndex = 0;

    for (int cycleCount = 0; cycleCount < cycles; requestIndex++, cycleCount++) {
        // If all request have been processed, exit the loop
        if (requestIndex >= numRequests) {
            break;
        }
        
        // If request exceeds number of cycles, signal the module to set the resultCycles to SIZE_MAX
        if (cycleCount == cycles - 1 && requestIndex < numRequests) {
            cache.requestsExceedCycles.write(1);
        }

        // Update request
        requestAddr = requests[requestIndex].addr;
        requestWE = requests[requestIndex].we;
        requestData = requests[requestIndex].data;
        
        // Run simulation for 1 cycle
        sc_start(1, SC_SEC);

        // If still waiting for latency, keep using the same request
        if (cache.waitForCacheLatency.read() || cache.waitForMemoryLatency.read()) {
            requestIndex--;
            continue;
        }
        
        // Only conduct tests once finished initializing main memory with matrix_multiplication.csv
        if (requests[requestIndex].we) {
            if (isInitializationFinished) {  
                // Calculate primitiveGateCount for addition  
                uint32_t result = entryMatrixC + mulResultTemp;

                if (c == MATRIX_SIZE - 1 && requestAddr != addressC[c_i][c_j]) {
                    std::cerr << "Error in Matrix C: Wrote to the wrong address" << endl;
                    std::cerr << "Expected: " << requestAddr << " but got: " << addressC[c_i][c_j] << endl;
                }
                if (c++ == MATRIX_SIZE - 1 && result != C[c_i][c_j]) {
                    std::cerr << "Error in Matrix C: Something went wrong while storing data" << endl;
                    std::cerr << "Expected: " << result << " but got: " << C[c_i][c_j] << endl;
                }

                if (c == MATRIX_SIZE) {
                    c_j++;
                    c = 0;
                }
                if (c_j == MATRIX_SIZE) {
                    c_j = 0;
                    c_i++;
                }
            }
        } else {
            isInitializationFinished = true;

            // Read from Matrix A
            if (readMatrixA) {
                entryMatrixA = cache.data.read();
                
                if (requestAddr != addressA[c_i][a_j]) {
                    std::cerr << "Error in Matrix A: Wrote to the wrong address" << endl;
                    std::cerr << "Expected: " << requestAddr << " but got: " << addressA[c_i][a_j] << endl;
                }
                if (entryMatrixA != A[c_i][a_j++]) {
                    std::cerr << "Error in Matrix A: Something went wrong while storing data" << endl;
                    std::cerr << "Expected: " << entryMatrixA << " but got: " << A[c_i][a_j - 1] << endl;
                }

                if (a_j == MATRIX_SIZE) {
                    a_j = 0;
                }
                readMatrixA = false;
                readMatrixB = true;
            }
            
            // Read from Matrix B
            else if (readMatrixB) {
                entryMatrixB = cache.data.read();  
                
                if (requestAddr != addressB[b_i][c_j]) {
                    std::cerr << "Error in Matrix B: Wrote to the wrong address" << endl;
                    std::cerr << "Expected: " << requestAddr << " but got: " << addressB[b_i][c_j] << endl;                    
                }
                if (entryMatrixB != B[b_i++][c_j]) {
                    std::cerr << "Error in Matrix B: Something went wrong while storing data" << endl;
                    std::cerr << "Expected: " << entryMatrixB << " but got: " << B[b_i - 1][c_j] << endl;                    
                }


                if (b_i == MATRIX_SIZE) {
                    b_i = 0;
                }
                
                readMatrixB = false;
                readMatrixC = true;
            } 
            
            // Read intermediate values from matrix C
            else if (readMatrixC) {
                entryMatrixC = cache.data.read();

                // Calculate primitiveGateCount for multiplication
                mulResultTemp = entryMatrixA * entryMatrixB;
                readMatrixC = false;
                readMatrixA = true;
            }
        }
    }

    // Update result
    result = cache.resultTemp;

    // Close and free resources
    if (simulationTracefileCreated) {
        sc_close_vcd_trace_file(simulationTracefile);
    }
    delete mainMemory;
    delete cache.cache;

    return result;
}
