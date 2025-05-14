#pragma once 

#include <cstdint>
#include <iostream>
#include <string>
#include "const.h"


class RAM {
    public:
        RAM() {
            std::fill(memory, memory + MEMORY_SIZE, 0);
        }

        ~RAM() {
            delete []memory;
        }

        void LoadByteInToRAM(uint32_t index, uint8_t byte) {
            memory[index] = byte;
        }


        uint8_t LoadByteFromRAM(uint32_t index) {
            return memory[index];
        }


    private:
        uint8_t* memory = new uint8_t[MEMORY_SIZE];
};