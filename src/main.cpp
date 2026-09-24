// main.cpp - Lee el archivo de comandos y ejecuta la simulación
#include "vm.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

static void usage(const char *prog) {
    std::printf("Uso: %s <archivo> [-m KB_memoria_fisica] [-v]\n", prog);
    std::printf("  -m  memoria física en KB (defecto y mínimo: 256)\n");
    std::printf("  -v  muestra cada operación (VA, PA, fallo/acierto)\n");
}

int main(int argc, char **argv) {
    const char *file = 0;
    unsigned long memKB = 256;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-v") verbose = true;
        else if (a == "-m" && i + 1 < argc) memKB = std::strtoul(argv[++i], 0, 10);
        else if (file == 0) file = argv[i];
        else { usage(argv[0]); return 1; }
    }
    if (file == 0) { usage(argv[0]); return 1; }
    if (memKB * 1024 < MIN_PHYS_BYTES) {
        std::printf("Error: la memoria física mínima es 256 KB\n");
        return 1;
    }

    std::ifstream in(file);
    if (!in) { std::printf("Error: no se pudo abrir '%s'\n", file); return 1; }

    VirtualMemory vm(static_cast<uint32_t>(memKB * 1024));
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

    // Se lee por "tokens": funciona con un comando por línea o todos en una línea.
    std::string cmd;
    while (in >> cmd) {
        if (cmd == "alloc") {
            unsigned long long bytes; in >> bytes;
            uint32_t start;
            if (bytes <= 0xFFFFFFFFULL && vm.alloc(static_cast<uint32_t>(bytes), start)) {
                if (verbose) std::printf("alloc %llu -> VA 0x%X\n", bytes, start);
            } else std::printf("Error: alloc %llu falló\n", bytes);
        } else if (cmd == "write" || cmd == "read" || cmd == "free") {
            std::string addrStr; in >> addrStr;
            unsigned long long addr = std::strtoull(addrStr.c_str(), 0, 0); // acepta decimal y 0x..
            if (addr > 0xFFFFFFFFULL) { std::printf("Error: VA fuera de 32 bits: %s\n", addrStr.c_str()); continue; }
            uint32_t va = static_cast<uint32_t>(addr);
            AccessInfo info;

            if (cmd == "write") {
                unsigned int value; in >> value;   // se guarda 1 byte (0-255)
                if (vm.write(va, static_cast<uint8_t>(value), info)) {
                    if (verbose) std::printf("write 0x%X = %u -> PA 0x%X  %s\n", va, value & 0xFF, info.pa, info.faulted ? "[FALLO]" : "[HIT]");
                } else std::printf("Error: write en VA 0x%X no reservada (segfault)\n", va);
            } else if (cmd == "read") {
                uint8_t value = 0;
                if (vm.read(va, value, info)) {
                    if (verbose) std::printf("read 0x%X -> %u (PA 0x%X)  %s\n", va, value, info.pa, info.faulted ? "[FALLO]" : "[HIT]");
                } else std::printf("Error: read en VA 0x%X no reservada (segfault)\n", va);
            } else {
                if (vm.freeRegion(va)) { if (verbose) std::printf("free 0x%X\n", va); }
                else std::printf("Error: free 0x%X no es inicio de una región\n", va);
            }
        } else {
            std::printf("Comando desconocido: %s\n", cmd.c_str());
        }
    }

    long long us = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::steady_clock::now() - t0).count();

    const Stats &s = vm.stats();
    double hit = s.accesses ? 100.0 * (s.accesses - s.faults) / s.accesses : 0.0;
    std::printf("===== Estadísticas =====\n");
    std::printf("Política: FIFO\n");
    std::printf("Memoria física: %lu KB (%u marcos)\n", memKB, vm.numFrames());
    std::printf("Total de accesos: %lu\n", s.accesses);
    std::printf("Total fallos de página: %lu\n", s.faults);
    std::printf("Hit rate: %.2f%%\n", hit);
    std::printf("Total reemplazos: %lu\n", s.replacements);
    std::printf("Tiempo de simulación: %lld us\n", us);
    return 0;
}
