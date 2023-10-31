#define _DEFAULT_SOURCE
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>

#include "myalloc.h"

node_t *_arena_start;

int statusno = 0;

int myinit(size_t size)
{
    int adjusted_size = 0;
    size_t neg = -1;

    if (size >= neg)
    {
        return ERR_BAD_ARGUMENTS;
    }

    if (size == getpagesize())
    {
        adjusted_size = size;
    }
    else
    {
        adjusted_size = size + (getpagesize() - (size % getpagesize()));
    }

    _arena_start = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    _arena_start->size = adjusted_size;
    _arena_start->fwd = NULL;
    _arena_start->bwd = NULL;
    _arena_start->is_free = 1;

    return _arena_start->size;
}

int mydestroy()
{
    if (_arena_start == NULL)
    {
        return ERR_UNINITIALIZED;
    }

    munmap(_arena_start, _arena_start->size);
    _arena_start = NULL;

    // check if munmap worked
    if (_arena_start == NULL)
    {
        return 0;
    }
    else
    {
        return ERR_CALL_FAILED;
    }
}

void *myalloc(size_t size)
{
    node_t *current = _arena_start;
    node_t *new_node = NULL;
    size_t neg = -1;

    // check for bad arguments
    if (size >= neg)
    {
        statusno = ERR_BAD_ARGUMENTS;
        return NULL;
    }

    // check if arena is initialized
    if (_arena_start == NULL)
    {
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    // check if there is sufficient space for the first allocation
    if (size + sizeof(node_t) > current->size && current->is_free == 1 && current->fwd == NULL)
    {
        statusno = ERR_OUT_OF_MEMORY;
        return NULL;
    }
    
    

    // check if size is 0
    while (current != NULL)
    {
        // check if the current node is free and big enough, if so, allocate, else, move to next node
        if (current->is_free == 1 && current->size >= size)
        {
            // check if the current node is big enough to split
            if (current->size > size + sizeof(node_t))
            {
                new_node = (node_t *)((char *)current + sizeof(node_t) + size);
                if (current->bwd == NULL){
                    new_node->size = current->size - size - (sizeof(node_t) * 2);
                }else{
                    new_node->size = current->size - size - (sizeof(node_t));
                }
                new_node->is_free = 1;
                new_node->fwd = current->fwd;
                new_node->bwd = current;
                current->fwd = new_node;
                current->size = size;
                current->is_free = 0;
                return (void *)((char *)current + sizeof(node_t));
            }
            else
            {
                // if the current node is not big enough to split, allocate the whole thing
                if(current->size - size >= sizeof(node_t)){ // check if there is enough space to split in the arena (including the header)
                    current->size = size;
                }
                current->is_free = 0;
                statusno = 0;
                return (void *)((char *)current + sizeof(node_t));
            }
            
        }
        else
        {
            current = current->fwd;
        }

        // check if we reached the end of the arena
        if (current == NULL)
        {
            statusno = ERR_OUT_OF_MEMORY;
            return NULL;
        }
    }
    return NULL;
}

void myfree(void *ptr)
{
    // check if arena is initialized
    if (_arena_start == NULL)
    {
        statusno = ERR_UNINITIALIZED;
        return;
    }

    // check if ptr is NULL
    if (ptr == NULL)
    {
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    // check if ptr is in the arena
    if (ptr < (void *)_arena_start || ptr > (void *)((char *)_arena_start + getpagesize()))
    {
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    // get the address of the chunk header
    node_t *chunk = (node_t *)((char *)ptr - sizeof(node_t));
    // check if the chunk is free
    if (chunk->is_free == 1)
    {
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    // if there are 2 or more contiguous chunks, merge them into one large chunk
    if (chunk->fwd != NULL && chunk->fwd->is_free == 1)
    {
        chunk->size = chunk->size + chunk->fwd->size + sizeof(node_t);
        chunk->fwd = chunk->fwd->fwd;
        if (chunk->fwd != NULL)
        {
            chunk->fwd->bwd = chunk;
        }
    }

    if (chunk->bwd != NULL && chunk->bwd->is_free == 1)
    {
        chunk->bwd->size = chunk->bwd->size + chunk->size + sizeof(node_t);
        chunk->bwd->fwd = chunk->fwd;
        if (chunk->fwd != NULL)
        {
            chunk->fwd->bwd = chunk->bwd;
        }
    }

    // free the chunk
    chunk->is_free = 1;
}

extern void* myalloc(size_t size){
    node_t *curr = _arena_start;
    

    while(1){//finding chuck big enough for 
        if(curr->size >= size && curr->is_free ==1){
            break;
        }

        if(curr->fwd != NULL){
            curr = curr->fwd;
        }else{//no other chunks left
            statusno = ERR_OUT_OF_MEMORY;
            return NULL;
        }
    }

    size_t sizeNeeded = size;//total size needed to allocate including header

    if(curr->size > sizeNeeded){
        //add split code here
        ;
    }else{//continue
        
    }


}
