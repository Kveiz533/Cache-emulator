#pragma once

#include "const.h"
#include "ram.h"
#include <cstdint>
#include <iomanip>
#include <stdio.h>
#include <vector>
#include <cmath>



class Cache_LRU {
    public:
        void LoadByteInToRAM(uint32_t index, uint8_t byte) {
            ram_.LoadByteInToRAM(index, byte);
        }

        uint32_t LoadByteFromRAM(uint32_t index) {
            return ram_.LoadByteFromRAM(index);
        }

        void LoadBytesInToRAM(uint64_t address, std::vector<uint8_t> data) {
            uint8_t tag = (address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN + CACHE_OFFSET_LEN) - 1);
            uint8_t index  = (address >> CACHE_OFFSET_LEN) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN) - 1);   
            uint8_t offset = address & (uint64_t) (std::pow(2, CACHE_OFFSET_LEN) - 1);  


            uint64_t ram_address = (address >> CACHE_OFFSET_LEN) << CACHE_OFFSET_LEN;
            bool is_found = false;
            int index_to_place = -1;
            int index_to_erase = -1;
            uint64_t biggest_time_since_use = 0;

            for (int i = 0; i < CACHE_WAY; i++) {
                if (cache_memory_[index][i].valid && cache_memory_[index][i].cache_tag == tag) { // нахождение строки с таким же тегом
                    is_found = true;

                    hit_data_++;

                    for (int j = offset; j < offset + data.size(); j++) { 
                        cache_memory_[index][i].cache_line_memory[j] = data[j - offset];
                    }

                    cache_memory_[index][i].dirty = true;
                    cache_memory_[index][i].time = -1;

                }

                if (!cache_memory_[index][i].valid && index_to_place == -1) { // нахождение свободного места
                    index_to_place = i;
                }

                if (cache_memory_[index][i].time >= biggest_time_since_use) { // нахождение самой старой строки 
                    biggest_time_since_use = cache_memory_[index][i].time;
                    index_to_erase = i;
                }

                cache_memory_[index][i].time++;
            }

            if (!is_found) {
                miss_data_++;
                

                int target_index = (index_to_place != -1) ? index_to_place : index_to_erase;
                
                if (target_index == index_to_erase && cache_memory_[index][target_index].dirty) { // вытеснение dirty-строки
                    UpdateCacheLine(index, target_index);
                }
    
                LoadCacheLine(index, target_index, ram_address); // загрузка строки
                
                for (int j = offset; j < offset + data.size(); j++) { // записываем данные
                    cache_memory_[index][target_index].cache_line_memory[j] = data[j - offset];
                }

                cache_memory_[index][target_index].dirty = true;
            }

        }

        std::vector<uint8_t> LoadBytesFromRAM(uint64_t address, int bytes_amount, bool is_instr = false) {
            uint8_t tag = (address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN + CACHE_OFFSET_LEN) - 1);
            uint8_t index  = (address >> CACHE_OFFSET_LEN) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN) - 1);   
            uint8_t offset = address & (uint64_t) (std::pow(2, CACHE_OFFSET_LEN) - 1);  


            uint64_t ram_address = (address >> CACHE_OFFSET_LEN) << CACHE_OFFSET_LEN;
            bool is_found = false;
            int index_to_place = -1;
            int index_to_erase = -1;
            uint64_t biggest_time_since_use = 0;

            std::vector<uint8_t> bytes(bytes_amount);

            for (int i = 0; i < CACHE_WAY; i++) {
                if (cache_memory_[index][i].valid && cache_memory_[index][i].cache_tag == tag) { // нахождение строки с таким же тегом
                    is_found = true;

                    if (is_instr) {
                        hit_instr_++;
                    } else {
                        hit_data_++;
                    }

                    for (int j = offset; j < offset + bytes_amount; j++) { 
                        bytes[j - offset] = cache_memory_[index][i].cache_line_memory[j];
                    }

                    cache_memory_[index][i].time = -1;
                }

                if (!cache_memory_[index][i].valid && index_to_place == -1) { // нахождение свободного места
                    index_to_place = i;
                }

                if (cache_memory_[index][i].time >= biggest_time_since_use) { // нахождение самой старой строки 
                    biggest_time_since_use = cache_memory_[index][i].time;
                    index_to_erase = i;
                }

                cache_memory_[index][i].time++;
            }

            if (!is_found) {

                if (is_instr) {
                    miss_instr_++;
                } else {
                    miss_data_++;
                }

                int target_index = (index_to_place != -1) ? index_to_place : index_to_erase;
                
                if (target_index == index_to_erase && cache_memory_[index][target_index].dirty) { // вытеснение dirty-строки
                    UpdateCacheLine(index, target_index);
                }
    
                LoadCacheLine(index, target_index, ram_address); // загрузка строки
                
                for (int j = offset; j < offset + bytes_amount; j++) { // записываем данные
                    bytes[j - offset] = cache_memory_[index][target_index].cache_line_memory[j];
                }
            }
    
            return bytes;
        }

        void PrintStatistic() {
            printf("replacement\thit rate\thit rate (inst)\thit rate (data)\n");
            printf("        LRU\t%3.5f%%\t%3.5f%%\t%3.5f%%\n", 
                FormatNan((double)(hit_data_ + hit_instr_) / (double)(hit_data_ + miss_data_ + hit_instr_ + miss_instr_) * 100),
                FormatNan((double)hit_instr_ / (double)(hit_instr_ + miss_instr_) * 100),
                FormatNan((double)hit_data_ / (double)(hit_data_ + miss_data_) * 100));
        }

        void WriteBackAll() {
            for (auto bucket: cache_memory_) {
                for (auto way: bucket) {
                    if (way.valid && way.dirty) {
                        for (int i = 0; i < CACHE_LINE_SIZE; i++) {
                            ram_.LoadByteInToRAM(
                                way.address + i,
                                way.cache_line_memory[i]
                            );
                        }
                    }
                }
            }
        }


    private:
        struct Cache_Line {
            bool dirty = false;
            bool valid = false;
            uint8_t cache_tag;
            uint32_t address;
            uint64_t time = 0;
            uint8_t cache_line_memory[CACHE_LINE_SIZE];
        };

        void UpdateCacheLine(uint32_t index, uint32_t way) {
            for (int i = 0; i < CACHE_LINE_SIZE; i++) {
                ram_.LoadByteInToRAM(
                    cache_memory_[index][way].address + i,
                    cache_memory_[index][way].cache_line_memory[i]
                );
            }
            cache_memory_[index][way].dirty = false;
        }
    
        void LoadCacheLine(uint32_t index, uint32_t way, uint64_t ram_address) {
            for (int i = 0; i < CACHE_LINE_SIZE; i++) {
                cache_memory_[index][way].cache_line_memory[i] = ram_.LoadByteFromRAM(ram_address + i);
            }

            cache_memory_[index][way].address = ram_address;
            cache_memory_[index][way].cache_tag = (ram_address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN + CACHE_OFFSET_LEN) - 1);;
            cache_memory_[index][way].valid = true;
            cache_memory_[index][way].dirty = false;
            cache_memory_[index][way].time = 0;
        }

        double FormatNan(double value) {
            if (std::isnan(value)) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            return value;
        }

        std::vector<std::vector<Cache_Line>> cache_memory_{
            CACHE_SET_COUNT,
        std::vector<Cache_Line>(CACHE_WAY)};

        size_t hit_data_ = 0;
        size_t miss_data_ = 0;

        size_t hit_instr_ = 0;
        size_t miss_instr_ = 0;
        
        RAM ram_;
};



class Cache_BitPLRU {
    public:
        void LoadByteInToRAM(uint32_t index, uint8_t byte) {
            ram_.LoadByteInToRAM(index, byte);
        }
        
        void LoadBytesInToRAM(uint64_t address, std::vector<uint8_t> data) {
            uint8_t tag = (address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN + CACHE_OFFSET_LEN) - 1);
            uint8_t index  = (address >> CACHE_OFFSET_LEN) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN) - 1);   
            uint8_t offset = address & (uint64_t) (std::pow(2, CACHE_OFFSET_LEN) - 1);  

            uint64_t ram_address = (address >> CACHE_OFFSET_LEN) << CACHE_OFFSET_LEN;
            bool is_found = false;
            int index_to_place = -1;
            int index_to_erase = -1;
            int all_one = 0;

            for (int i = 0; i < CACHE_WAY; i++) {
                if (cache_memory_[index][i].valid && cache_memory_[index][i].cache_tag == tag) { // нахождение строки с таким же тегом
                    is_found = true;

                    hit_data_++;

                    for (int j = offset; j < offset + data.size(); j++) { 
                        cache_memory_[index][i].cache_line_memory[j] = data[j - offset];
                    }

                    cache_memory_[index][i].dirty = true;
                    cache_memory_[index][i].mru_bit = 1;

                }

                if (!cache_memory_[index][i].valid && index_to_place == -1) { // нахождение свободного места
                    index_to_place = i;
                }

                if (index_to_erase == -1 && cache_memory_[index][i].mru_bit == 0) { // нахождение самой первой строки с 0 
                    index_to_erase = i;
                }

                all_one += cache_memory_[index][i].mru_bit;
            }

            if (!is_found) {
                miss_data_++;

                int target_index = (index_to_place != -1) ? index_to_place : index_to_erase;

                if (all_one == CACHE_WAY) {
                    target_index = 0;
                    index_to_erase = 0;

                    for (int i = 0; i < CACHE_WAY; i++) {
                        cache_memory_[index][i].mru_bit = 0;
                    }
                }
                
                if (target_index == index_to_erase && cache_memory_[index][target_index].dirty) { // вытеснение dirty-строки
                    UpdateCacheLine(index, target_index);
                }
    
                LoadCacheLine(index, target_index, ram_address); // загрузка строки
                
                for (int j = offset; j < offset + data.size(); j++) { // записываем данные
                    cache_memory_[index][target_index].cache_line_memory[j] = data[j - offset];
                }

                cache_memory_[index][target_index].dirty = true;
            }

        }

        std::vector<uint8_t> LoadBytesFromRAM(uint64_t address, int bytes_amount, bool is_instr = false) {
            uint8_t tag = (address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN + CACHE_OFFSET_LEN) - 1);
            uint8_t index  = (address >> CACHE_OFFSET_LEN) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN) - 1);   
            uint8_t offset = address & (uint64_t) (std::pow(2, CACHE_OFFSET_LEN) - 1);  

            uint64_t ram_address = (address >> CACHE_OFFSET_LEN) << CACHE_OFFSET_LEN;
            bool is_found = false;
            int index_to_place = -1;
            int index_to_erase = -1;

            int all_one = 0;

            std::vector<uint8_t> bytes(bytes_amount);

            for (int i = 0; i < CACHE_WAY; i++) {
                if (cache_memory_[index][i].valid && cache_memory_[index][i].cache_tag == tag) { // нахождение строки с таким же тегом
                    is_found = true;

                    if (is_instr) {
                        hit_instr_++;
                    } else {
                        hit_data_++;
                    }

                    for (int j = offset; j < offset + bytes_amount; j++) { 
                        bytes[j - offset] = cache_memory_[index][i].cache_line_memory[j];
                    }

                    cache_memory_[index][i].mru_bit = 1;
                }

                if (!cache_memory_[index][i].valid && index_to_place == -1) { // нахождение свободного места
                    index_to_place = i;
                }

                if (index_to_erase == -1 && cache_memory_[index][i].mru_bit == 0) { // нахождение самой первой строки с 0 
                    index_to_erase = i;
                }

                all_one += cache_memory_[index][i].mru_bit;
            }

            if (!is_found) {

                if (is_instr) {
                    miss_instr_++;
                } else {
                    miss_data_++;
                }

                int target_index = (index_to_place != -1) ? index_to_place : index_to_erase;

                if (all_one == CACHE_WAY) {
                    target_index = 0;
                    index_to_erase = 0;

                    for (int i = 0; i < CACHE_WAY; i++) {
                        cache_memory_[index][i].mru_bit = 0;
                    }
                }
                
                if (target_index == index_to_erase && cache_memory_[index][target_index].dirty) { // вытеснение dirty-строки
                    UpdateCacheLine(index, target_index);
                }
    
                LoadCacheLine(index, target_index, ram_address); // загрузка строки
                
                for (int j = offset; j < offset + bytes_amount; j++) { // записываем данные
                    bytes[j - offset] = cache_memory_[index][target_index].cache_line_memory[j];
                }
            }
    
            return bytes;
        }

        void PrintStatistic() {
            printf("      bpLRU\t%3.5f%%\t%3.5f%%\t%3.5f%%\n",
                FormatNan((double)(hit_data_ + hit_instr_) / (double)(hit_data_ + miss_data_ + hit_instr_ + miss_instr_) * 100),
                FormatNan((double)hit_instr_ / (double)(hit_instr_ + miss_instr_) * 100),
                FormatNan((double)hit_data_ / (double)(hit_data_ + miss_data_) * 100));

        }


        void WriteBackAll() {
            for (auto bucket: cache_memory_) {
                for (auto way: bucket) {
                    if (way.valid && way.dirty) {
                        for (int i = 0; i < CACHE_LINE_SIZE; i++) {
                            ram_.LoadByteInToRAM(
                                way.address + i,
                                way.cache_line_memory[i]
                            );
                        }
                    }
                }
            }
        }


    private:
        struct Cache_Line {
            bool dirty = false;
            bool valid = false;
            uint8_t cache_tag;
            uint32_t address;
            uint8_t mru_bit = 0;
            uint8_t cache_line_memory[CACHE_LINE_SIZE];
        };

        void UpdateCacheLine(uint32_t index, uint32_t way) {
            for (int i = 0; i < CACHE_LINE_SIZE; i++) {
                ram_.LoadByteInToRAM(
                    cache_memory_[index][way].address + i,
                    cache_memory_[index][way].cache_line_memory[i]
                );
            }
            cache_memory_[index][way].dirty = false;
            cache_memory_[index][way].mru_bit = 1;
        }
    
        void LoadCacheLine(uint32_t index, uint32_t way, uint64_t ram_address) {
            for (int i = 0; i < CACHE_LINE_SIZE; i++) {
                cache_memory_[index][way].cache_line_memory[i] = ram_.LoadByteFromRAM(ram_address + i);
            }

            cache_memory_[index][way].address = ram_address;
            cache_memory_[index][way].cache_tag = (ram_address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN)) & (uint64_t) (std::pow(2, CACHE_INDEX_LEN + CACHE_OFFSET_LEN) - 1);
            cache_memory_[index][way].valid = true;
            cache_memory_[index][way].dirty = false;
            cache_memory_[index][way].mru_bit = 1;
        }

        double FormatNan(double value) {
            if (std::isnan(value)) {
                return std::numeric_limits<double>::quiet_NaN();
            }
            return value;
        }

        std::vector<std::vector<Cache_Line>> cache_memory_{
            CACHE_SET_COUNT,
        std::vector<Cache_Line>(CACHE_WAY)};

        size_t hit_data_ = 0;
        size_t miss_data_ = 0;

        size_t hit_instr_ = 0;
        size_t miss_instr_ = 0;
        
        RAM ram_;
};