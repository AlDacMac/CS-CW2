/*************************************************************************************|
|   1. YOU ARE NOT ALLOWED TO SHARE/PUBLISH YOUR CODE (e.g., post on piazza or online)|
|   2. Fill main.c and memory_hierarchy.c files                                       |
|   3. Do not use any other .c files neither alter main.h or parser.h                 |
|   4. Do not include any other library files                                         |
|*************************************************************************************/
#include "mipssim.h"

/// @students: declare cache-related structures and variables here

int offsetBits = 4;
int noLines;
int indexBits;


typedef struct{
    int valid;
    int tag;
    uint32_t *data;
} cacheLine;

cacheLine **cache;

/*Previously used functions for calcuating values from address - I don't think I can actually do
    this without a header file
//Gets a cache index from an address
int getIndex(int address){
    return get_piece_of_a_word(address, (uint8_t) (32 - indexBits), (uint8_t) indexBits);
}

//Gets a cache tag from an address
int getTag(int address){
    return get_piece_of_a_word(address, (uint8_t) offsetBits,
            (uint8_t) (32 - indexBits - offsetBits));
}

//Gets a cache offset (for getting the word from within a block) from an address
int offset(int address){
    return get_piece_of_a_word(address, 0, 4);
}

//Finds which block in the memory the address points to
//  - When loading blocks into the cache, we do not start the block at the position
//    given but instead treat the memory as if it were already split into discrete
//    blocks of four, and load the block that the address is contained in
//      - hence the existance of a calculatable offset
int getBlockPosition(int address){
    return (address - offset(address));
}*/

void memory_state_init(struct architectural_state* arch_state_ptr) {
    arch_state_ptr->memory = (uint32_t *) malloc(sizeof(uint32_t) * MEMORY_WORD_NUM);
    memset(arch_state_ptr->memory, 0, sizeof(uint32_t) * MEMORY_WORD_NUM);
    if(cache_size == 0){
        // CACHE DISABLED
        memory_stats_init(arch_state_ptr, 0); // WARNING: we initialize for no cache 0
    }else {
        // CACHE ENABLED
        noLines = cache_size / 16;
        printf("noLines = %d\n", noLines);
        indexBits = ceil(log2(noLines));
        printf("indexBits = %d\n", indexBits);
        cache = (cacheLine**) malloc(sizeof(cacheLine*) * noLines);
        for(int i = 0; i < noLines; i++){
            cache[i] = malloc(sizeof(cacheLine));
            cache[i]->valid = 0;
            cache[i]->data = malloc(sizeof(uint32_t) * 4);
        }
        memory_stats_init(arch_state_ptr, indexBits);
        /// @students: memory_stats_init(arch_state_ptr, X); <-- fill # of tag bits for cache 'X' correctly
    }
}

// TODO what happens when we load from a block at the end of memory?

// returns data on memory[address / 4]
int memory_read(int address){
    arch_state.mem_stats.lw_total++;
    check_address_is_word_aligned(address);

    if(cache_size == 0){
        // CACHE DISABLED
        return (int) arch_state.memory[address / 4];
    }else{
        printf("entered cache load: ");
        // CACHE ENABLED
        int offset = get_piece_of_a_word(address, 0, offsetBits);
        printf("offset = %d, ", offset);
        int idx = get_piece_of_a_word(address, (uint8_t) offsetBits
                , (uint8_t) indexBits);
        printf("idx = %d, ", idx);
        int tag = get_piece_of_a_word(address, (uint8_t) offsetBits + indexBits,
                (uint8_t) (32 - indexBits - offsetBits));
        printf("tag = %d, ", tag);
        int blockStart = (address - offset);
        printf("blockStart = %d\n", blockStart);
        cacheLine *line = cache[idx];
        // If hit return data
        if(line->tag == tag && line->valid){
            printf("    - got a hit\n");
            arch_state.mem_stats.lw_cache_hits++;
            return (int) line->data[offset / 4];
        }
        // Else load data into the cache, then return it
        else{
            printf("    - got a miss\n");
            for(int i = 0; i < 4; i++){
                int loadingOffset = 4 * i;
                int loadingAddress = blockStart + loadingOffset;
                line->data[i] = arch_state.memory[loadingAddress/4];
            }
            line->tag = tag;
            line->valid = 1;
            return (int) line->data[offset / 4];
        }
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.lw_cache_hits
    }
    return 0;
}



// writes data on memory[address / 4]
void memory_write(int address, int write_data){
    arch_state.mem_stats.sw_total++;
    check_address_is_word_aligned(address);

    if(cache_size == 0){
        // CACHE DISABLED
        arch_state.memory[address / 4] = (uint32_t) write_data;
    }else{
        // CACHE ENABLED
        int offset = get_piece_of_a_word(address, 0, offsetBits);
        int idx = get_piece_of_a_word(address, (uint8_t) offsetBits
                , (uint8_t) indexBits);
        int tag = get_piece_of_a_word(address, (uint8_t) offsetBits + indexBits
                , (uint8_t) (32 - indexBits - offsetBits));
        int blockStart = (address - offset);
        cacheLine *line = cache[idx];
        if(line->tag == tag && line->valid){
            arch_state.mem_stats.sw_cache_hits++;
            line->data[offset] = (uint32_t) write_data;
            arch_state.memory[address / 4] = (uint32_t) write_data;
        }
        else{
            arch_state.memory[address / 4] = (uint32_t) write_data;
        }
        /// @students: your implementation must properly increment: arch_state_ptr->mem_stats.sw_cache_hits
    }
}
