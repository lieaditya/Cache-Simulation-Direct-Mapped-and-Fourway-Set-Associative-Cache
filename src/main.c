#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../includes/io_structs.hpp"

extern Result run_simulation(int cycles, bool directMapped, unsigned cacheLines, unsigned cacheLineSize, 
                            unsigned cacheLatency, int memoryLatency, size_t numRequests, 
                            Request requests[], const char* tracefile);

const char* usageMsg = 
    "Usage: %s [options] <csv-path>\n"
    "\nOptions:\n"
    "-c, --cycles <value>        Number of simulated cycles.\n"
    "--directmapped              Simulates a direct-mapped cache.\n"
    "--fourway                   Simulates a four-way-associative cache.\n"
    "--cacheline-size <value>    Size for each cachelines in Byte.\n"
    "--cachelines <value>        Number of cachelines.\n"
    "--cache-latency <value>     Latency for cache in cycles.\n"
    "--memory-latency <value>    Latency for main memory in cycles.\n"
    "--tf=<tracefile_name>       A tracefile containing all signals from the simulation. (leave this empty for no Tracefile)\n"
    "<csv-path>                  Path to .csv file that contains the simulation's inputs.\n"
    "-h, --help                  Prints a short description of the program's options and a usage example.\n\n";
        
const char* helpMsg = 
    "Here are 2 examples how to run the simulation:\n"
    "\nout/simulation --cycles 40 --fourway --cacheline-size 8 --cachelines 8 --cache-latency 2 --memory-latency 3 --tf=tracefile out/inputs.csv\n"
    "This initializes a 4-way-associative cache simulation with 40 cycles, cacheline size of 8 Bytes, 8 cachelines, with a cache latency of 2 cycles and a memory latency of 3 cycles.\n"
    "A tracefile with the name 'tracefile' will be generated and the .csv path containing the inputs is located at out/inputs.csv\n"
    "\nout/simulation --cycles 50 --directmapped --cacheline-size 4 --cachelines 16 --cache-latency 1 --memory-latency 4 out/inputs.csv\n"
    "This initializes a direct-mapped cache simulation with 50 cycles, cacheline size of 4 Bytes, 16 cachelines, with a cache latency of 1 cycle and a memory latency of 4 cycles.\n"
    "A tracefile won't be generated and the .csv path containing the inputs is located at out/inputs.csv\n";

void print_usage(const char* progname) {
    fprintf(stderr, usageMsg, progname);
}

void print_help(const char* progname) {
    print_usage(progname);
    fprintf(stderr, "\n%s", helpMsg);
}

int fetch_num(char* parameterName) {
    char* endptr;
    int numInput = strtol(optarg, &endptr, 10);

    // If endptr is not null-byte, then the optarg value is invalid
    if (endptr[0] != '\0') {
        fprintf(stderr, "Invalid value for %s!\n", parameterName);
        exit(EXIT_FAILURE);
    }
    return numInput;
}

char* read_csv(const char* csvPath) {
    FILE* csvFile = fopen(csvPath, "r");

    // Edge-case null file
    if (csvFile == NULL) {
        fprintf(stderr, "Error, can't open .csv file!\n");
        return NULL;
    }

    struct stat fileInfo;

    // Edge-case read file informations
    if (fstat(fileno(csvFile), &fileInfo)) {
        fprintf(stderr, "Error retrieving .csv file stat\n");
        fclose(csvFile);
        return NULL;
    }

    // Edge-case invalid file
    if (!S_ISREG(fileInfo.st_mode) || fileInfo.st_size <= 0) {
        fprintf(stderr, "Error processing .csv file\n");
        fclose(csvFile);
        return NULL;
    }

    // Allocate memory in heap
    char* content = (char*) malloc(fileInfo.st_size + 1);
    if (!content) {
        fprintf(stderr, "Error reading .csv file, cannot allocate enough memory\n");
        fclose(csvFile);
        return NULL;
    }

    // Read file content and save it in "content"
    if (fread(content, 1, fileInfo.st_size, csvFile) != (size_t) fileInfo.st_size) {
        fprintf(stderr, "Error reading .csv file!\n");
        free(content);
        content = NULL;
        fclose(csvFile);
        return NULL;
    }

    // Add null-terminating byte at the end
    content[fileInfo.st_size] = '\0';
    return content;
}

int count_num_of_request(const char* content) {
    if (content == NULL) {
        return 0;
    }

    int count = 0;
    const char* line = content;

    while ((line = strchr(line, '\n')) != NULL) {
        count++;
        line++;
    }

    if (strlen(content) > 0 && content[strlen(content) - 1] != '\n') {
        count++;
    }

    return count;
}

void parse_data(const char* content, struct Request request[], int numberOfRequests, int* linesRead) {
    // Copy of the content to avoid modifying the original
    char* copyOfContent = strdup(content);

    int counter = 0;

    // Edge-case null content
    if (copyOfContent == NULL) {
        fprintf(stderr, "Error, .csv file does not have any content.\n");
        exit(EXIT_FAILURE);
    }
    
    char* rest;
    char* line = strtok_r(copyOfContent, "\n", &rest);
    *linesRead = 0;

    for (int i = 0; i < numberOfRequests && line != NULL; i++, counter++) {
        // Allocate space for "W" or "R" and null-byte terminator
        char tempWE[2];

        unsigned int addr;
        int data;

        sscanf(line, "%1s,%x,%d", tempWE, &addr, &data);
        data = tempWE[0] == 'W' ? data : 0;

        request[i].we = tempWE[0] == 'W' ? 1 : 0;
        request[i].addr = addr;
        request[i].data = data;

        line = strtok_r(NULL, "\n", &rest);
        (*linesRead)++;
    }
    printf(".csv line counter: %d\n", counter);
    free(copyOfContent);
}

int main(int argc, char* const argv[]) {
    const char* progname = argv[0];
    
    // Edge-case missing options (executing program without options --> argc = 1)
    if (argc == 1) {
        fprintf(stderr, "Error! Options are missing.\n");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    int optionIndex = 0;
    int cycles = 0;
    bool directMapped = false;
    bool fourway = false;
    int cacheLineSize = 0;
    int cacheLines = 0;
    int cacheLatency = 0;
    int memoryLatency = 0;
    char* tracefile = "";
    bool isTracefilePassed = false;
    char* csvPath = "";
    bool isCSVPassed = false;
    char* CSVContent;
    size_t numRequests = 0;
    Request* requests;
    int linesRead = 0;

    struct option longOptions[] = {
        {"cycles", required_argument, 0, 'c'},
        {"directmapped", no_argument, 0, 0},
        {"fourway", no_argument, 0, 0},
        {"cacheline-size", required_argument, 0, 0},
        {"cachelines", required_argument, 0, 0},
        {"cache-latency", required_argument, 0, 0},
        {"memory-latency", required_argument, 0, 0},
        {"tf", required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "c:h", longOptions, &optionIndex)) != -1) {
        switch (opt) {
        case 'c':
            int fetchedNumber = fetch_num("cycles");
            if (fetchedNumber <= 0) {
                fprintf(stderr, "Error! Cycles should be a positive value.\n");
                exit(EXIT_FAILURE);
            }
            cycles = fetchedNumber;
            break;
        case 'h':
            print_help(progname);
            return 0;
        case 0:
            if (strcmp(longOptions[optionIndex].name, "directmapped") == 0 || strcmp(longOptions[optionIndex].name, "fourway") == 0) {
                 if (strcmp(longOptions[optionIndex].name, "directmapped") == 0) {
                    directMapped = true;
                } else {
                    fourway = true;
                }

                if (directMapped && fourway) {
                    fprintf(stderr, "Error! Cache can't be direct mapped and 4-way associative at the same time.\n");
                    exit(EXIT_FAILURE);
                } 
            }
            
            if (strcmp(longOptions[optionIndex].name, "cacheline-size") == 0) {
                int fetchedNumber = fetch_num("cacheline-size");
                if (fetchedNumber <= 0) {
                    fprintf(stderr, "Error! Cacheline size should be larger than 0.\n");
                    exit(EXIT_FAILURE);
                } else if (fetchedNumber % 4 != 0) {
                    fprintf(stderr, "Error! Cacheline size should be multiple of 4.\n");
                    exit(EXIT_FAILURE);
                }
                cacheLineSize = fetchedNumber;
            } 
            
            if (strcmp(longOptions[optionIndex].name, "cachelines") == 0) {
                int fetchedNumber = fetch_num("cachelines");
                if (fetchedNumber <= 0) {
                    fprintf(stderr, "Error! Number of cachelines should be larger than 0.\n");
                    exit(EXIT_FAILURE);
                } else if (fourway && (fetchedNumber < 4 || fetchedNumber % 4 != 0)) {
                    fprintf(stderr, "Error! For 4-way associative cache, cachelines value should be multiple of 4.\n");
                    exit(EXIT_FAILURE);
                } else if (directMapped && (fetchedNumber & (fetchedNumber - 1)) != 0) {
                    fprintf(stderr, "Error! For direct-mapped cache, cachelines value should be power of two.\n");
                    exit(EXIT_FAILURE);
                }
                cacheLines = fetchedNumber;
            }
            
            if (strcmp(longOptions[optionIndex].name, "cache-latency") == 0) {
                int fetchedNumber = fetch_num("cache-latency");
                if (fetchedNumber <= 0) {
                    fprintf(stderr, "Error! Cache latency value should be greater than 0.\n");
                    exit(EXIT_FAILURE);
                }
                cacheLatency = fetchedNumber;
            }
            
            if (strcmp(longOptions[optionIndex].name, "memory-latency") == 0) {
                int fetchedNumber = fetch_num("memory-latency");
                if (fetchedNumber <= 0) {
                    fprintf(stderr, "Error! Memory latency value should be greater than 0.\n");
                    exit(EXIT_FAILURE);
                }
                memoryLatency = fetchedNumber;
            }
            
            if (strcmp(longOptions[optionIndex].name, "tf") == 0) {
                tracefile = optarg;
                isTracefilePassed = true;
            }
            break;
        default:
            print_usage(progname);
            exit(EXIT_FAILURE);
        }
    }

    // Remaining arguments goes to csvPath
    if (optind < argc) {
        csvPath = argv[optind];
        isCSVPassed = true;
    }

    // Check if all options have been initialized
    if (cycles == 0 || directMapped == fourway || cacheLineSize == 0 || cacheLines == 0 || cacheLatency == 0 || memoryLatency == 0 || !isCSVPassed) {
        fprintf(stderr, "Error! Not all options have been correctly initialized!\n");
        fprintf(stderr, "Type <program name> -h or --help for options.\n");
        exit(EXIT_FAILURE);
    }

    // Check if csvPath is passed
    if (csvPath) {
        CSVContent = read_csv(csvPath);
        if (!CSVContent) {
            fprintf(stderr, "Error reading .csv file.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, ".csv file path not provided.\n");
        exit(EXIT_FAILURE);
    }

    printf("User Input:\n");
    printf("Cycles: %d\n", cycles);
    printf("Direct mapped: %d\n", directMapped);
    printf("Fourway: %d\n", fourway);
    printf("Cacheline Size: %d\n", cacheLineSize);
    printf("Cachelines: %d\n", cacheLines);
    printf("Cache Latency: %d\n", cacheLatency);
    printf("Memory Latency: %d\n", memoryLatency);
    printf("Tracefile Name: %s\n", tracefile);
    printf("Path to .csv file: %s\n", csvPath);
    
    numRequests = count_num_of_request(CSVContent);
    requests = (Request *) malloc(numRequests * sizeof(Request));

    // Allocate memory in heap for requests array
    if (requests == NULL) {
        fprintf(stderr, "Error allocating memory for requests array.\n");
        free(CSVContent);
        exit(EXIT_FAILURE);
    }

    parse_data(CSVContent, requests, numRequests, &linesRead);

    Result result = run_simulation(cycles, directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency, numRequests, requests, tracefile);

    printf("\nSimulation Results: \n");
    printf("Cycles: %zu\n", result.cycles);
    printf("Misses: %zu\n", result.misses);
    printf("Hits: %zu\n", result.hits);
    printf("Primitive Gate Count: %zu\n", result.primitiveGateCount);

    // Free resources
    free(CSVContent);
    free(requests);

    return 0;
}