#ifndef FOURWAYLRUCACHE_HPP
#define FOURWAYLRUCACHE_HPP

#include <unordered_map>
#include <cstdint>
#include <vector>

#include "address_structs.hpp"
#include "io_structs.hpp"
#include "cache_base.hpp"
#include "main_memory.hpp"

using namespace std;

class LRUCache {
private:
    class Node {
    public:
        uint8_t* data;
        uint32_t tagAsMapKey;
        bool isFirstTime;
        
        Node* next;
        Node* prev;

        Node(uint32_t numberOfOffsetBits);
        ~Node();
    };

    unordered_map<uint32_t, Node*> map;
    Node* head; // head = MRU
    Node* tail; // tail = LRU
    
    void update_to_mru(Node* node);
    void add_node(Node* node);
    void remove_node(Node* node);
        
public:
    LRUCache(CacheConfig cacheConfig);
    
    ~LRUCache();

    uint32_t read_from_cache(uint32_t address, CacheAddress cacheAddress, CacheConfig cacheConfig, Result &result);

    void write_to_cache(uint32_t address, CacheAddress cacheAddress, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result);

    void replace_lru(uint32_t address, uint32_t cacheAddressTag, CacheConfig cacheConfig);
};

class FourWayLRUCache : public CacheBase {
private:
    vector<LRUCache*> cacheSets;

public:
    FourWayLRUCache(CacheConfig cacheConfig);

    ~FourWayLRUCache();

    uint32_t read_from_cache(uint32_t address, CacheConfig cacheConfig, Result &result) override;
    void write_to_cache(uint32_t address, CacheConfig cacheConfig, uint32_t dataToWrite, Result &result) override;
};

#endif
