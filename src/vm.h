#ifndef VM_H
#define VM_H

#include <cstdint>
#include <deque>
#include <map>
#include <vector>

//PT1 (10 bits), PT2 (10 bits), Offset (12 bits)
const uint32_t OFFSET_BITS   = 12;
const uint32_t PAGE_SIZE     = 1u << OFFSET_BITS;   //4096 bytes
const uint32_t TABLE_ENTRIES = 1024;                //2^10 entradas por tabla
const uint32_t MIN_PHYS_BYTES = 256 * 1024;         //mínimo: 256 KB

struct PTE {
    uint32_t frame     = 0;      
    bool valid         = false;
    bool accessed      = false;
    bool dirty         = false;
    bool allocated     = false;
};

struct Level2Table {
    PTE entries[TABLE_ENTRIES];
};

struct Stats {
    unsigned long accesses     = 0;
    unsigned long faults       = 0;
    unsigned long replacements = 0;
};

struct AccessInfo {
    uint32_t pa      = 0;
    bool     faulted = false;
};

class VirtualMemory {
public:
    explicit VirtualMemory(uint32_t physBytes);
    ~VirtualMemory();

    bool alloc(uint32_t bytes, uint32_t &startVA);
    bool freeRegion(uint32_t va);
    bool write(uint32_t va, uint8_t value, AccessInfo &info);
    bool read(uint32_t va, uint8_t &value, AccessInfo &info);

    // Traducir de VA a PA
    bool translate(uint32_t va, bool isWrite, AccessInfo &info);

    const Stats &stats() const { return st; }
    uint32_t numFrames() const { return nFrames; }

private:
    VirtualMemory(const VirtualMemory &);
    VirtualMemory &operator=(const VirtualMemory &);

    PTE *getPTE(uint32_t vpn, bool create);         
    uint32_t handlePageFault(uint32_t vpn, PTE &pte); 
    uint32_t selectVictimFIFO();                      
    void evictFrame(uint32_t frame);                  

    Level2Table *level1[TABLE_ENTRIES];        
    std::vector<uint8_t> physMem;             
    uint32_t nFrames;                         
    std::vector<uint32_t> frameOwner;         
    std::vector<uint32_t> freeFrames;          
    std::deque<uint32_t> fifoQueue;            
    std::map<uint32_t, std::vector<uint8_t> > swapSpace; // "disco": VPN -> contenido
    std::map<uint32_t, uint32_t> regions;      
    uint64_t nextVA;                           
    Stats st;
};

#endif
