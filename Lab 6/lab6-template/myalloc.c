#define _DEFAULT_SOURCE
#include <stddef.h>
#include <sys/mman.h>


#include "myalloc.h"

node_t *_arena_start;

int myinit(size_t size){
    int adjusted_size;

    if(size <= 0){
        return ERR_BAD_ARGUMENTS;
    }

    adjusted_size = size + (4096 - (size % 4096));

    _arena_start = mmap(NULL, adjusted_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    
    _arena_start->size = adjusted_size;
    _arena_start->fwd = NULL;
    _arena_start->bwd = NULL;
    _arena_start->is_free = 1;

    return adjusted_size;
}

int mydestroy(){
    munmap(_arena_start, _arena_start->size);
    return 0;
}