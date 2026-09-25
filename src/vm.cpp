#include "vm.h"
#include <algorithm>

VirtualMemory::VirtualMemory(uint32_t physBytes)
    : physMem(physBytes, 0), nFrames(physBytes / PAGE_SIZE),
      frameOwner(nFrames, 0), nextVA(0) {
    for (uint32_t i = 0; i < TABLE_ENTRIES; i++) level1[i] = 0;
    // Todos los marcos empiezan libres (el marco 0 se entrega primero)
    for (uint32_t f = nFrames; f > 0; f--) freeFrames.push_back(f - 1);
}

VirtualMemory::~VirtualMemory() {
    for (uint32_t i = 0; i < TABLE_ENTRIES; i++) delete level1[i];
}

//Devuelve la PTE de una página virtual. Si create es true y la tabla de
//nivel 2 no existe, la crea dinámicamente.
PTE *VirtualMemory::getPTE(uint32_t vpn, bool create) {
    uint32_t pt1 = vpn >> 10;
    uint32_t pt2 = vpn & (TABLE_ENTRIES - 1);
    if (level1[pt1] == 0) {
        if (!create) return 0;
        level1[pt1] = new Level2Table();
    }
    return &level1[pt1]->entries[pt2];
}

//Traducción
bool VirtualMemory::translate(uint32_t va, bool isWrite, AccessInfo &info) {
    uint32_t vpn    = va >> OFFSET_BITS;
    uint32_t offset = va & (PAGE_SIZE - 1);

    PTE *pte = getPTE(vpn, false);
    if (pte == 0 || !pte->allocated) return false; 

    st.accesses++;
    info.faulted = false;
    if (!pte->valid) {                               
        st.faults++;
        info.faulted = true;
        handlePageFault(vpn, *pte);
    }
    pte->accessed = true;
    if (isWrite) pte->dirty = true;

    info.pa = pte->frame * PAGE_SIZE + offset;
    return true;
}

//Busca un marco (libre o por reemplazo) y carga la página en él.
uint32_t VirtualMemory::handlePageFault(uint32_t vpn, PTE &pte) {
    uint32_t frame;
    if (!freeFrames.empty()) {
        frame = freeFrames.back();
        freeFrames.pop_back();
    } else {
        frame = selectVictimFIFO();   // política de reemplazo
        evictFrame(frame);
        st.replacements++;
    }

    //Cargar contenido desde el "swap" si es que ya existia
    uint8_t *dst = &physMem[frame * PAGE_SIZE];
    std::map<uint32_t, std::vector<uint8_t> >::iterator it = swapSpace.find(vpn);
    if (it != swapSpace.end())
        std::copy(it->second.begin(), it->second.end(), dst);
    else
        std::fill(dst, dst + PAGE_SIZE, 0);

    pte.frame    = frame;
    pte.valid    = true;
    pte.accessed = false;
    pte.dirty    = false;
    frameOwner[frame] = vpn;
    fifoQueue.push_back(frame);       // entra al final de la cola FIFO
    return frame;
}

//Reemplazo
// Los aciertos (hits) NO alteran el orden de la cola.
uint32_t VirtualMemory::selectVictimFIFO() {
    uint32_t victim = fifoQueue.front();
    fifoQueue.pop_front();
    return victim;
}

//Saca la página que ocupa 'frame' y si estaba modificada se guarda en el swap.
void VirtualMemory::evictFrame(uint32_t frame) {
    uint32_t vpn = frameOwner[frame];
    PTE *pte = getPTE(vpn, false);
    if (pte->dirty) {
        uint8_t *src = &physMem[frame * PAGE_SIZE];
        swapSpace[vpn] = std::vector<uint8_t>(src, src + PAGE_SIZE);
    }
    pte->valid = false;
}

//Operaciones principales
bool VirtualMemory::alloc(uint32_t bytes, uint32_t &startVA) {
    if (bytes == 0) return false;
    uint64_t pages = (static_cast<uint64_t>(bytes) + PAGE_SIZE - 1) / PAGE_SIZE;
    if (nextVA + pages * PAGE_SIZE > (1ULL << 32)) return false;  // se acabó el espacio de 32 bits

    startVA = static_cast<uint32_t>(nextVA);
    uint32_t firstVpn = startVA >> OFFSET_BITS;
    for (uint64_t i = 0; i < pages; i++)
        getPTE(firstVpn + static_cast<uint32_t>(i), true)->allocated = true;

    regions[startVA] = static_cast<uint32_t>(pages);
    nextVA += pages * PAGE_SIZE;
    return true;
}

bool VirtualMemory::freeRegion(uint32_t va) {
    std::map<uint32_t, uint32_t>::iterator r = regions.find(va);
    if (r == regions.end()) return false;

    uint32_t firstVpn = va >> OFFSET_BITS;
    for (uint32_t i = 0; i < r->second; i++) {
        uint32_t vpn = firstVpn + i;
        PTE *pte = getPTE(vpn, false);
        if (pte->valid) {   // devolver el marco y quitarlo de la cola FIFO
            fifoQueue.erase(std::find(fifoQueue.begin(), fifoQueue.end(), pte->frame));
            freeFrames.push_back(pte->frame);
        }
        *pte = PTE();       // limpia todos los bits
        swapSpace.erase(vpn);
    }
    regions.erase(r);
    return true;
}

bool VirtualMemory::write(uint32_t va, uint8_t value, AccessInfo &info) {
    if (!translate(va, true, info)) return false;
    physMem[info.pa] = value;
    return true;
}

bool VirtualMemory::read(uint32_t va, uint8_t &value, AccessInfo &info) {
    if (!translate(va, false, info)) return false;
    value = physMem[info.pa];
    return true;
}
