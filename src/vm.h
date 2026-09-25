#ifndef VM_H
#define VM_H

#include <cstdint>
#include <deque>
#include <map>
#include <vector>

// ---- Constantes de la arquitectura simulada (VA de 32 bits) ----
// | PT1 (10 bits) | PT2 (10 bits) | Offset (12 bits) |
const uint32_t OFFSET_BITS   = 12;
const uint32_t PAGE_SIZE     = 1u << OFFSET_BITS;   // 4096 bytes
const uint32_t TABLE_ENTRIES = 1024;                // 2^10 entradas por tabla
const uint32_t MIN_PHYS_BYTES = 256 * 1024;         // mínimo exigido: 256 KB

// Entrada de tabla de páginas (PTE)
struct PTE {
    uint32_t frame     = 0;      // número de página física (marco)
    bool valid         = false;  // true = la página está en memoria física
    bool accessed      = false;  // se leyó o escribió alguna vez desde que se cargó
    bool dirty         = false;  // se escribió desde que se cargó
    bool allocated     = false;  // la página pertenece a una región reservada con alloc
};

// Tabla de nivel 2: se crea dinámicamente solo cuando se necesita
struct Level2Table {
    PTE entries[TABLE_ENTRIES];
};

// Estadísticas de la simulación
struct Stats {
    unsigned long accesses     = 0;
    unsigned long faults       = 0;
    unsigned long replacements = 0;
};

// Resultado de un acceso (para poder imprimir el detalle)
struct AccessInfo {
    uint32_t pa      = 0;
    bool     faulted = false;
};

class VirtualMemory {
public:
    explicit VirtualMemory(uint32_t physBytes);
    ~VirtualMemory();

    // Reserva 'bytes' de memoria virtual (redondeado a páginas). Devuelve la VA inicial.
    bool alloc(uint32_t bytes, uint32_t &startVA);
    // Libera la región que empieza exactamente en 'va'.
    bool freeRegion(uint32_t va);
    // Acceso de escritura / lectura de 1 byte.
    bool write(uint32_t va, uint8_t value, AccessInfo &info);
    bool read(uint32_t va, uint8_t &value, AccessInfo &info);

    // Traducción VA -> PA (dispara el fallo de página si hace falta).
    bool translate(uint32_t va, bool isWrite, AccessInfo &info);

    const Stats &stats() const { return st; }
    uint32_t numFrames() const { return nFrames; }

private:
    // No se permite copiar (tiene punteros propios)
    VirtualMemory(const VirtualMemory &);
    VirtualMemory &operator=(const VirtualMemory &);

    PTE *getPTE(uint32_t vpn, bool create);          // recorre las 2 tablas
    uint32_t handlePageFault(uint32_t vpn, PTE &pte); // obtiene marco y carga la página
    uint32_t selectVictimFIFO();                      // política de reemplazo
    void evictFrame(uint32_t frame);                  // saca la página del marco

    Level2Table *level1[TABLE_ENTRIES];        // tabla de nivel 1
    std::vector<uint8_t> physMem;              // memoria física simulada
    uint32_t nFrames;                          // cantidad de marcos
    std::vector<uint32_t> frameOwner;          // marco -> VPN que lo ocupa
    std::vector<uint32_t> freeFrames;          // marcos libres
    std::deque<uint32_t> fifoQueue;            // marcos en orden de llegada (front = más viejo)
    std::map<uint32_t, std::vector<uint8_t> > swapSpace; // "disco": VPN -> contenido
    std::map<uint32_t, uint32_t> regions;      // VA inicial -> número de páginas
    uint64_t nextVA;                           // siguiente VA libre (asignador simple)
    Stats st;
};

#endif
