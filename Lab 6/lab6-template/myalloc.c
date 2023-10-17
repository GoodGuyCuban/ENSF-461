#define _DEFAULT_SOURCE
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>


#include "myalloc.h"

node_t *_arena_start;

int myinit(size_t size){
    int adjusted_size = 0;
    size_t neg = -1;

    if(size >= neg){
        return ERR_BAD_ARGUMENTS;
    }

    if(size == getpagesize()){
        adjusted_size = size;
    }else{
        adjusted_size = size + (getpagesize() - (size % getpagesize()));
    }

    _arena_start = mmap(NULL, adjusted_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    
    _arena_start->size = adjusted_size;
    _arena_start->fwd = NULL;
    _arena_start->bwd = NULL;
    _arena_start->is_free = 1;

    return _arena_start->size;
}

int mydestroy(){
    if(_arena_start == NULL){
        return ERR_UNINITIALIZED;
    }

    munmap(_arena_start, _arena_start->size);
    _arena_start = NULL;


    //check if munmap worked
    if(_arena_start == NULL){
        return 0;
    }else{
        return ERR_CALL_FAILED;
    }
}