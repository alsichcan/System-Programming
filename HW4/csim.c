// Jisang Park 2017-15108 
#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>


// Command Line Arguments
int v; // Optional verbose flag that displays trace info
int s; // Number of set index bits (S = 2^s is the number of sets)
int E; // Associativity (number of lines per set)
int b; // Number of block bits (B = 2^b is the block size)
char* trace; // Name of the valgrind trace to display

// Cache Counts
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;

// Cache Structure
typedef struct{
    int valid_bit;
    int tag;
    int counter; // Least Recently Used Counter (The Bigger, More Recent)
} cache_line;

cache_line** cache;

int accessCNT = 0; // To manage LRU counter

void cacheAccess(unsigned long long address){
    // Cache : tag (t bits) + set index (s bits) + block offset (b bits)
    int tag = address >> (s+b); // Left-most bits
    int set_index = (address >> b) - (tag << s);


    // Iterate to check Cache Hit
    for(int i = 0; i < E; i++){
        if(cache[set_index][i].valid_bit && cache[set_index][i].tag == tag){
            // Cache Hit
            if(v) printf(" hit");
            cache[set_index][i].counter = accessCNT++;
            hit_count++;
            return;
        }
    }

    // Cache Miss
    if(v) printf(" miss");
    miss_count++;

    // Variables for LRU replacement method
    accessCNT++;
    int oldest_cnt = accessCNT; // To store the least recently used counter
    int oldest_idx = 0; // To store the least recently used cache line

    // Iterate Update the least recently used cache line
    for(int i = 0; i < E; i++){
        if(cache[set_index][i].counter < oldest_cnt){
            oldest_cnt = cache[set_index][i].counter;
            oldest_idx = i;
        }
    }

    // If LRU cache line is valid, then evict
    if(cache[set_index][oldest_idx].valid_bit){
        if(v) printf(" eviction");
        eviction_count++;
    }

    cache[set_index][oldest_idx].valid_bit = 1;
    cache[set_index][oldest_idx].tag = tag;
    cache[set_index][oldest_idx].counter = accessCNT++;
}


int main(int argc, char **argv)
{
    // Parse Command Line
    int opt;
    /* looping over arguments */
    while(-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))){
        /* determine which argument it's processing */
        switch(opt){
            case 'h': break;
            case 'v': v= 1; break;
            case 's': s = atoi(optarg); break;
            case 'E': E = atoi(optarg); break;
            case 'b': b = atoi(optarg); break;
            case 't': trace = optarg; break;
            default: exit(1);
        }
    }

    // Cache Init
    int S = 1 << s; // Number of sets

    cache = (cache_line**)malloc(sizeof(cache_line*) * S); 

    for(int i = 0; i < S; i++){
        cache[i] = (cache_line*)malloc(sizeof(cache_line) * E);
        for(int j = 0; j < E; j++){
            cache[i][j].valid_bit = 0;
            cache[i][j].tag = 0;
            cache[i][j].counter = 0;
        }
    }

    // Open Trace File
    FILE *pFile; // pointer to FILE object
    pFile = fopen(trace, "r");
    
    char identifier;
    unsigned long long address;
    int size;
    // Reading lines like " M 20, 1" or "L 19, 3"
    while(fscanf(pFile, " %c %llx, %d", &identifier, &address, &size) > 0){
        if(v) printf("%c %llx, %d", identifier, address, size);
        switch(identifier){
            case 'I': break;
            case 'L': cacheAccess(address); break;
            case 'M': cacheAccess(address); cacheAccess(address); break;
            case 'S': cacheAccess(address); break;
            default: break;
        }
        printf("\n");
    }

    printSummary(hit_count, miss_count, eviction_count);
    fclose(pFile); // remember to close file when done

    // Free Malloced Cache
    for(int i = 0; i < S; i ++){
        free(cache[i]);
    }
    free(cache);

    return 0;
}
