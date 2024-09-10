#include <iostream>
#include <unordered_map>
#include <cmath>

#include "../includes/four_way_lru_cache.hpp"
#include "../includes/main_memory_global.hpp"

using namespace std;

LRUCache::Node::Node(uint32_t numberOfOffsetBits) : next(nullptr), prev(nullptr) {
    data = new uint8_t[static_cast<uint32_t>(pow(2, numberOfOffsetBits))];
    isFirstTime = true;
}

LRUCache::Node::~Node() {
    delete[] data;
}

void LRUCache::update_to_mru(Node* node) {
    remove_node(node);
    add_node(node);
}

void LRUCache::add_node(Node* node) {
    node->next = head->next;
    node->next->prev = node;
    node->prev = head;
    head->next = node;
}

void LRUCache::remove_node(Node* node) {
    Node* prev = node->prev;
    Node* next = node->next;
    prev->next = next;
    next->prev = prev;
}

LRUCache::LRUCache(CacheConfig cacheConfig) {
    // Create dummy head and tail
    head = new Node(cacheConfig.numberOfOffsetBits);
    tail = new Node(cacheConfig.numberOfOffsetBits);
    head->next = tail;
    tail->prev = head;

    // Add 4 nodes in between head and tail
    for (int i = 0; i < cacheConfig.numberOfTagBits; i++) {
        Node* node = new Node(cacheConfig.numberOfOffsetBits);
        node->tagAsMapKey = i; 
        add_node(node);
        map[i] = node;
    }
}

LRUCache::~LRUCache() {
    Node* current = head;
    while (current != nullptr) {
        Node* next = current->next;
        delete current;
        current = next;
    }
}

uint32_t LRUCache::read_from_cache(uint32_t address, CacheAddress cacheAddress, CacheConfig cacheConfig, Result &result) {
    // Replace if the tag isn't in the map or if it's a cold miss, and update number of misses/hits
    bool found = true;
    if (map.find(cacheAddress.tag) == map.end()) {
        replace_lru(address, cacheAddress.tag, cacheConfig);
        result.misses++;
        found = false;
    } else if (map[cacheAddress.tag]->isFirstTime) {
        replace_lru(address, cacheAddress.tag, cacheConfig);
        result.misses++;
        found = false;
    }
    if (found) {
        result.hits++;
    }

    // Merge 4 bytes of data from 4 consecutive offsets into a single 32-bit-unsigned integer
    Node* node = map[cacheAddress.tag]; 
    uint8_t data1 = node->data[cacheAddress.offset];
    uint8_t data2 = node->data[cacheAddress.offset + 1];
    uint8_t data3 = node->data[cacheAddress.offset + 2];
    uint8_t data4 = node->data[cacheAddress.offset + 3];
    uint32_t dataToRead = CacheBase::merge_data_to_uint32(data1, data2, data3, data4);

    update_to_mru(node);

    return dataToRead;
}

void LRUCache::write_to_cache(uint32_t address, CacheAddress cacheAddress, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result) {
    // Replace if the tag isn't in the map or if it's a cold miss, and update number of misses/hits
    bool found = true;
    if (map.find(cacheAddress.tag) == map.end()) {
        replace_lru(address, cacheAddress.tag, cacheConfig);
        result.misses++;
        found = false;
    } else if (map[cacheAddress.tag]->isFirstTime) {
        replace_lru(address, cacheAddress.tag, cacheConfig);
        result.misses++;
        found = false;
    }
    if (found) {
        result.hits++;
    }

    // Split a 32-bit-unsigned integer into 4 bytes of data with consecutive offsets according to little-endian
    Node* node = map[cacheAddress.tag];

    uint8_t byteOfData = static_cast<uint8_t>(dataToWrite & 0xFF);
    node->data[cacheAddress.offset] = byteOfData;
    mainMemory->write_to_ram(address, byteOfData);

    byteOfData = static_cast<uint8_t>((dataToWrite >> 8) & 0xFF);
    node->data[cacheAddress.offset + 1] = byteOfData;
    mainMemory->write_to_ram(address + 1, byteOfData);

    byteOfData = static_cast<uint8_t>((dataToWrite >> 16) & 0xFF);
    node->data[cacheAddress.offset + 2] = byteOfData;
    mainMemory->write_to_ram(address + 2, byteOfData);

    byteOfData = static_cast<uint8_t>((dataToWrite >> 24) & 0xFF);
    node->data[cacheAddress.offset + 3] = byteOfData;
    mainMemory->write_to_ram(address + 3, byteOfData);

    update_to_mru(node);
}

void LRUCache::replace_lru(uint32_t address, uint32_t cacheAddressTag, CacheConfig cacheConfig) {
    // Update the LRU node with a new node with the correct attributes
    Node* LRUNode = tail->prev;
    Node* newNode = new Node(cacheConfig.numberOfOffsetBits);

    newNode->next = tail;
    newNode->prev = LRUNode->prev;
    LRUNode->prev->next = newNode;
    tail->prev = newNode;

    newNode->tagAsMapKey = cacheAddressTag;
    newNode->isFirstTime = false;

    map.erase(LRUNode->tagAsMapKey);
    map[cacheAddressTag] = newNode;
    delete LRUNode;

    // Fetch a block of data from the main memory
    uint32_t totalOffset = static_cast<uint32_t>(pow(2, cacheConfig.numberOfOffsetBits));
    uint32_t startAddressToFetch = (address / totalOffset) * totalOffset;
    uint32_t lastAddressToFetch = startAddressToFetch + totalOffset - 1;
    
    for (uint32_t RAMAddress = startAddressToFetch, offset = 0; RAMAddress <= lastAddressToFetch; RAMAddress++, offset++) { // O(1)
        newNode->data[offset] = mainMemory->read_from_ram(RAMAddress);
    }
}

FourWayLRUCache::FourWayLRUCache(CacheConfig cacheConfig) {
    // Instantiate number of LRU Caches based index bits
    for (uint32_t i = 0; i < static_cast<uint32_t>(pow(2, cacheConfig.numberOfIndexBits)); i++) {
        cacheSets.push_back(new LRUCache(cacheConfig));
    }
}

FourWayLRUCache::~FourWayLRUCache() {
    for (auto LRUCache : cacheSets) {
        delete LRUCache;
    }
}

uint32_t FourWayLRUCache::read_from_cache(uint32_t address, CacheConfig cacheConfig, Result &result) {
    // Read from correct cache based on the calculated set
    CacheAddress cacheAddress(address, cacheConfig);
    uint32_t setIndex = cacheAddress.index;
    return cacheSets[setIndex]->read_from_cache(address, cacheAddress, cacheConfig, result);
}

void FourWayLRUCache::write_to_cache(uint32_t address, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result) {
    // Write to the correct cache based on the calculated set
    CacheAddress cacheAddress(address, cacheConfig);
    uint32_t setIndex = cacheAddress.index;
    cacheSets[setIndex]->write_to_cache(address, cacheAddress, cacheConfig, dataToWrite, result);
}
