#include "cache.h"
#include "processor.h"
#include <cstdint>
#include <cstring>


int main(int argc, char** argv) {
    std::string input_file, output_file;
    uint32_t start_address, size;
    bool recieve_start_adress = false;
    bool recieve_size = false;
    bool recieve_output_file = false;
    
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-i") == 0) {
            i++;
            input_file = argv[i];
        } else if (std::strcmp(argv[i], "-o") == 0) {
            recieve_output_file= true;
            i++;
            output_file = argv[i];
        } else if (!recieve_start_adress) {
            start_address = std::stoul(argv[i]);
            recieve_start_adress = true;
        } else if (!recieve_size) {
            size = std::stoul(argv[i]);
            recieve_size = true;
        }
    }

    Processor<Cache_LRU> processor_lru_cache(input_file);
    processor_lru_cache.ParseBinFiles();
    processor_lru_cache.Emulate();

    Processor<Cache_BitPLRU> processor_bitplru_cache(input_file);
    processor_bitplru_cache.ParseBinFiles();
    processor_bitplru_cache.Emulate();

    if (recieve_output_file) {
        processor_lru_cache.WriteIntoFile(output_file, start_address, size);
    }
    
    return 0;
}