#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "cache.h"


template<typename Cache_type>
class Processor {
public:
    Processor(const std::string& bin_filename) : bin_filename_(bin_filename) {}

    ~Processor() {
        delete []registers_;
    }

    void ParseBinFiles() {
        std::ifstream file(bin_filename_, std::ios::binary);

        for (int i = 0; i < 32; ++i) {
            uint32_t reg_value;
            file.read(reinterpret_cast<char*>(&reg_value), sizeof(reg_value));
            registers_[i] = reg_value;
        }

        pc_ = registers_[0];
        registers_[0] = 0; 
        uint8_t* buffer = new uint8_t[4];

        while (file.peek() != EOF) {
            uint32_t address = 0, size = 0;

            file.read(reinterpret_cast<char*>(&address), sizeof(address));
            file.read(reinterpret_cast<char*>(&size), sizeof(size));

            for (int j = 0; j < size / 4; j++) {
                file.read(reinterpret_cast<char*>(buffer), 4);

                for (uint32_t i = 0; i < 4; i++) {
                    cache_.LoadByteInToRAM(address + i + j * 4 , buffer[i]);
                }
            }

        }

        delete []buffer;
    }

    void Emulate() {
        while (registers_[1] != pc_) { // пока ra != pc
            std::vector<uint8_t> arr = cache_.LoadBytesFromRAM(pc_, 4, true);
            std::reverse(arr.begin(), arr.end());
            std::string bin_str;

            for (int i = 0; i < 4; ++i) {
                bin_str += std::bitset<8>(arr[i]).to_string();
            }

            if (bin_str == "00000000000000000000000001110011" || bin_str == "00000000000100000000000001110011") { // ecall ebreak
                break;
            }

            std::reverse(bin_str.begin(), bin_str.end());

            std::string operation_type = Slice(bin_str, 0, 6);
            std::string rd = Slice(bin_str, 7, 11);
            std::string imm = Slice(bin_str, 12, 31);

            if (operation_type == "0110111") { // lui
                registers_[std::stoul(rd, nullptr, 2)] = Sext(std::stoul(imm, nullptr, 2) << 12, 32); 
                pc_ += 4;
            } else if (operation_type == "0010111") { // auipc
                registers_[std::stoul(rd, nullptr, 2)] = pc_ + Sext(std::stoul(imm, nullptr, 2) << 12, 32); 
                pc_ += 4;
            }

            if (operation_type == "0001111") { // fence/fence.i
                pc_ += 4;
            }

            if (operation_type == "0010011") {
                std::string extra_operation_type = Slice(bin_str, 12, 14);
                std::string rs1 = Slice(bin_str, 15, 19);
                imm = Slice(bin_str, 20, 31);

                if (extra_operation_type == "000") { // addi
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] + Sext(std::stoul(imm, nullptr, 2), 12);
                } else if (extra_operation_type == "010") { // slti
                    registers_[std::stoul(rd, nullptr, 2)] = static_cast<int32_t>(registers_[std::stoul(rs1, nullptr, 2)]) < static_cast<int32_t>(Sext(std::stoul(imm, nullptr, 2), 12));                
                } else if (extra_operation_type == "011") { // sltiu
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] < Sext(std::stoul(imm, nullptr, 2), 12); 
                } else if (extra_operation_type == "100") { // xori
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] ^ Sext(std::stoul(imm, nullptr, 2), 12); 
                } else if (extra_operation_type == "110") { // ori
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] | Sext(std::stoul(imm, nullptr, 2), 12); 
                } else if (extra_operation_type == "111") { // andi
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] & Sext(std::stoul(imm, nullptr, 2), 12); 
                } 

                pc_ += 4;
            }



            if (operation_type == "0010011") {
                std::string extra_operation_type = Slice(bin_str, 12, 14);
                std::string rs1 = Slice(bin_str, 15, 19);
                std::string shamt = Slice(bin_str, 20, 24);
                std::string last_five_bits = Slice(bin_str, 27, 31);
                
                if (extra_operation_type == "001") { // slli
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] << std::stoul(shamt, nullptr, 2);
                } else if (extra_operation_type == "101" && last_five_bits == "00000") { // srli
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] >> std::stoul(shamt, nullptr, 2);
                } else if (extra_operation_type == "101" && last_five_bits == "01000") { // srai
                    registers_[std::stoul(rd, nullptr, 2)] = static_cast<int32_t>(registers_[std::stoul(rs1, nullptr, 2)]) >> std::stoul(shamt, nullptr, 2);
                }
            }

            if (operation_type == "0110011") {
                std::string extra_operation_type = Slice(bin_str, 12, 14);
                std::string last_five_bits = Slice(bin_str, 27, 31);
                std::string last_seven_bits = Slice(bin_str, 25, 31);

                std::string rs1 = Slice(bin_str, 15, 19);
                std::string rs2 = Slice(bin_str, 20, 24);

                if (extra_operation_type == "000" && last_seven_bits == "0000001") { // mul
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] * registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "001" && last_seven_bits == "0000001") { // mulh
                    registers_[std::stoul(rd, nullptr, 2)] = (int32_t) (((long long) registers_[std::stoul(rs1, nullptr, 2)] * (long long) registers_[std::stoul(rs2, nullptr, 2)]) >> 32);
                } else if (extra_operation_type == "010" && last_seven_bits == "0000001") { // mulhsu
                    registers_[std::stoul(rd, nullptr, 2)] = (int32_t) (((long long) registers_[std::stoul(rs1, nullptr, 2)] * (unsigned long long) registers_[std::stoul(rs2, nullptr, 2)]) >> 32);
                } else if (extra_operation_type == "011" && last_seven_bits == "0000001") { // mulhu
                    registers_[std::stoul(rd, nullptr, 2)] = (uint32_t) (((unsigned long long) registers_[std::stoul(rs1, nullptr, 2)] * (long long) registers_[std::stoul(rs2, nullptr, 2)]) >> 32);
                } else if (extra_operation_type == "100" && last_seven_bits == "0000001") { // div
                    registers_[std::stoul(rd, nullptr, 2)] = (uint32_t) ((long long) registers_[std::stoul(rs1, nullptr, 2)] / (long long) registers_[std::stoul(rs2, nullptr, 2)]);
                } else if (extra_operation_type == "101" && last_seven_bits == "0000001") { // divu
                    registers_[std::stoul(rd, nullptr, 2)] = (uint32_t) ((unsigned long long) registers_[std::stoul(rs1, nullptr, 2)] / (unsigned long long) registers_[std::stoul(rs2, nullptr, 2)]);
                } else if (extra_operation_type == "110" && last_seven_bits == "0000001") { // rem
                    registers_[std::stoul(rd, nullptr, 2)] = (uint32_t) ((long long) registers_[std::stoul(rs1, nullptr, 2)] % (long long) registers_[std::stoul(rs2, nullptr, 2)]);
                } else if (extra_operation_type == "111" && last_seven_bits == "0000001") { // remu
                    registers_[std::stoul(rd, nullptr, 2)] = (uint32_t) ((unsigned long long) registers_[std::stoul(rs1, nullptr, 2)] % (unsigned long long) registers_[std::stoul(rs2, nullptr, 2)]);
                } else if (extra_operation_type == "000" && last_five_bits == "00000") { // add
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] + registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "000" && last_five_bits == "01000") { // sub
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] - registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "001" && last_five_bits == "00000") { // sll
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] << registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "010" && last_five_bits == "00000") { // slt
                    registers_[std::stoul(rd, nullptr, 2)] = static_cast<int32_t>(registers_[std::stoul(rs1, nullptr, 2)]) < static_cast<int32_t>(registers_[std::stoul(rs2, nullptr, 2)]);
                } else if (extra_operation_type == "011" && last_five_bits == "00000") { // sltu
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] < registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "100" && last_five_bits == "00000") { // xor
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] ^ registers_[std::stoul(rs2, nullptr, 2)];                
                } else if (extra_operation_type == "101" && last_five_bits == "00000") { // srl
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] >> registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "101" && last_five_bits == "01000") { // sra
                    registers_[std::stoul(rd, nullptr, 2)] = static_cast<int32_t>(registers_[std::stoul(rs1, nullptr, 2)]) >> registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "110" && last_five_bits == "00000") { // or
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] | registers_[std::stoul(rs2, nullptr, 2)];
                } else if (extra_operation_type == "111" && last_five_bits == "00000") { // and
                    registers_[std::stoul(rd, nullptr, 2)] = registers_[std::stoul(rs1, nullptr, 2)] & registers_[std::stoul(rs2, nullptr, 2)];
                }

                pc_ += 4;
            }

            if (operation_type == "0000011") {
                std::string extra_operation_type = Slice(bin_str, 12, 14);
                std::string rs1 = Slice(bin_str, 15, 19);
                std::string offset = Slice(bin_str, 20, 31);
                std::string imm = Slice(bin_str, 20, 31);

                uint32_t address = registers_[std::stoi(rs1, nullptr, 2)] + Sext(std::stoul(imm, nullptr, 2), 12);

                if (extra_operation_type == "000") { // lb
                    registers_[std::stoul(rd, nullptr, 2)] = Sext(std::stoul(VectorToBinString(cache_.LoadBytesFromRAM(address, 1)), nullptr, 2), 8);
                } else if (extra_operation_type == "001") { // lh
                    registers_[std::stoul(rd, nullptr, 2)] = Sext(std::stoul(VectorToBinString(cache_.LoadBytesFromRAM(address, 2)), nullptr, 2), 16);
                } else if (extra_operation_type == "010") { // lw
                    registers_[std::stoul(rd, nullptr, 2)] = Sext(std::stoul(VectorToBinString(cache_.LoadBytesFromRAM(address, 4)), nullptr, 2), 32);
                } else if (extra_operation_type == "100") { // lbu
                    registers_[std::stoul(rd, nullptr, 2)] = std::stoul(VectorToBinString(cache_.LoadBytesFromRAM(address, 1)), nullptr, 2);
                } else if (extra_operation_type == "101") { // lhu
                    registers_[std::stoul(rd, nullptr, 2)] = std::stoul(VectorToBinString(cache_.LoadBytesFromRAM(address, 2)), nullptr, 2);
                }

                pc_ += 4;
            }

            if (operation_type == "0100011") {
                std::string extra_operation_type = Slice(bin_str, 12, 14);
                std::string rs1 = Slice(bin_str, 15, 19);
                std::string rs2 = Slice(bin_str, 20, 24);
                std::string offset = Slice(bin_str, 25, 31) + Slice(bin_str, 7, 11);

                if (extra_operation_type == "000") { // sb
                    std::vector<uint8_t> data(1);
                    data[0] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[0];
                    cache_.LoadBytesInToRAM(registers_[std::stoul(rs1, nullptr, 2)] + Sext(std::stoul(offset, nullptr, 2), 12), data);
                } else if (extra_operation_type == "001") { // sh
                    std::vector<uint8_t> data(2);
                    data[0] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[1];
                    data[1] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[0];
                    cache_.LoadBytesInToRAM(registers_[std::stoul(rs1, nullptr, 2)] + Sext(std::stoul(offset, nullptr, 2), 12), data);
                } else if (extra_operation_type == "010") { // sw
                    std::vector<uint8_t> data(4);

                    data[0] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[3];
                    data[1] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[2];
                    data[2] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[1];
                    data[3] = Uint32ToBytes(registers_[std::stoul(rs2, nullptr, 2)])[0];

                    cache_.LoadBytesInToRAM(registers_[std::stoul(rs1, nullptr, 2)] + Sext(std::stoul(offset, nullptr, 2), 12), data);
                }

                pc_ += 4;
            }

            if (operation_type == "1101111") { // jal
                std::string offset = Slice(bin_str, 31, 31) + Slice(bin_str, 12, 19) + Slice(bin_str, 20, 20) + Slice(bin_str, 21, 30)  + "0";
                registers_[std::stoul(rd, nullptr, 2)] = pc_ + 4;
                pc_ += Sext(std::stoul(offset, nullptr, 2), 21);
            }

            if (operation_type == "1100111") { // jalr
                std::string offset = Slice(bin_str, 20, 31);
                std::string rs1 = Slice(bin_str, 15, 19);

                uint32_t t = pc_ + 4;
                pc_ = (registers_[std::stoul(rs1, nullptr, 2)] + Sext(std::stoul(offset, nullptr, 2), 12)) & ~1;
                registers_[std::stoul(rd, nullptr, 2)] = t;
            }

            if (operation_type == "1100011") {  
                std::string extra_operation_type = Slice(bin_str, 12, 14);
                std::string rs1 = Slice(bin_str, 15, 19);
                std::string rs2 = Slice(bin_str, 20, 24);
                std::string offset = Slice(bin_str, 31, 31) + Slice(bin_str, 7, 7) + Slice(bin_str, 25, 30) + Slice(bin_str, 8, 11)  + "0";

                if (extra_operation_type == "000") { // beq
                    pc_ += (registers_[std::stoul(rs1, nullptr, 2)] == registers_[std::stoul(rs2, nullptr, 2)]) ? Sext(std::stoul(offset, nullptr, 2), 13) : 4;
                } else if (extra_operation_type == "001") { // bne
                    pc_ += (registers_[std::stoul(rs1, nullptr, 2)] != registers_[std::stoul(rs2, nullptr, 2)]) ? Sext(std::stoul(offset, nullptr, 2), 13) : 4;
                } else if (extra_operation_type == "100") { // blt
                    pc_ += (static_cast<int32_t>(registers_[std::stoul(rs1, nullptr, 2)]) < static_cast<int32_t>(registers_[std::stoul(rs2, nullptr, 2)])) ? Sext(std::stoul(offset, nullptr, 2), 13) : 4;
                } else if (extra_operation_type == "101") { // bge
                    pc_ += (static_cast<int32_t>(registers_[std::stoul(rs1, nullptr, 2)]) >= static_cast<int32_t>(registers_[std::stoul(rs2, nullptr, 2)])) ? Sext(std::stoul(offset, nullptr, 2), 13) : 4;
                } else if (extra_operation_type == "110") { // bltu
                    pc_ += (registers_[std::stoul(rs1, nullptr, 2)] < registers_[std::stoul(rs2, nullptr, 2)]) ? Sext(std::stoul(offset, nullptr, 2), 13) : 4;
                } else if (extra_operation_type == "111") { // bgeu
                    pc_ += (registers_[std::stoul(rs1, nullptr, 2)] >= registers_[std::stoul(rs2, nullptr, 2)]) ? Sext(std::stoul(offset, nullptr, 2), 13) : 4;
                }
            }
        } 

        cache_.WriteBackAll();
        cache_.PrintStatistic();
    }


    void WriteIntoFile(std::string& output_file, uint32_t start_address, uint32_t size) {
        std::ofstream out(output_file, std::ios::binary);

        out.write(reinterpret_cast<const char*>(&pc_), sizeof(pc_));

        for (int i = 1; i < 32; ++i) {
            out.write(reinterpret_cast<const char*>(&registers_[i]), sizeof(registers_[i]));
        }

        out.write(reinterpret_cast<const char*>(&start_address), sizeof(start_address));
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));

        for (uint32_t i = start_address; i < start_address + size; i++) {
            uint8_t byte = cache_.LoadByteFromRAM(i);
            out.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
        }

        out.close();
    }

private:
    std::string Slice(std::string& line, int index_st, int index_fn) {
        std::string res = "";

        for (int i = index_fn; i >= index_st; i--) {
            res += line[i];
        }

        return res;
    }

    int32_t Sext(const uint32_t& value, size_t original_size) {
        int shift = 32 - original_size;
        return static_cast<int32_t>(value << shift) >> shift;
    }

    std::vector<uint8_t> Uint32ToBytes(uint32_t value) {
        std::vector<uint8_t> bytes;

        bytes.push_back(static_cast<uint8_t>(value));   
        bytes.push_back(static_cast<uint8_t>(value >> 8));
        bytes.push_back(static_cast<uint8_t>(value >> 16));
        bytes.push_back(static_cast<uint8_t>(value >> 24)); 
        
        return bytes;
    }

    std::string VectorToBinString(const std::vector<uint8_t>& data) {
        std::string result;
        
        for (uint8_t byte : data) {
            for (int i = 7; i >= 0; i--) {
                if (byte & (1 << i)) {
                    result += "1";
                } else {
                    result += "0";
                }
            }
        }
        return result;
    }

    std::string bin_filename_;
    uint32_t* registers_ = new uint32_t[32];
    uint32_t pc_ = 0;
    Cache_type cache_;
};
