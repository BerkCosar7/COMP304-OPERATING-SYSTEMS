#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h> 

struct block{
	size_t size;//how many bytes beyond this block have been allocated in heap
	struct block *next; // where is the next block in linkedlist
	int isFree; //is this memory free
	struct block *next_free;
};



struct block *free_stack = NULL;
struct block *heap_start=NULL;
struct block *heap_end=NULL;


void addToFreeStack(struct block *blockToAdd) {

    if (!blockToAdd) {
        return;
    }


    if (!free_stack) {
        free_stack = blockToAdd;
        blockToAdd->next_free = NULL;
        return;
    }


    struct block *lastBlock = free_stack;
    while (lastBlock->next_free) {
        lastBlock = lastBlock->next_free;
    }


    lastBlock->next_free = blockToAdd;
    blockToAdd->next_free = NULL;
}







bool isMergable() {

	struct block *current=heap_start;
	while(current && current->next){
		if(current->next->isFree && current->isFree){
			return true;
		}else{
			current=current->next;
		}
	}
return false;

}


bool isInFreeStack(struct block *check){

	struct block *curr= free_stack;
	while(curr){
		if(curr==check){
			return true;
		}else{
			curr=curr->next_free;
		}
	}
	return false;


}

void removeAndAdd(struct block *to_remove) {

  
   struct block *prev = NULL;
   struct block *current=free_stack;


 while (current != NULL && current != to_remove) {
        prev = current;
        current = current->next_free;
    }

    if (current == to_remove) {
        if (prev == NULL) {
            free_stack = to_remove->next_free;
        } else {
            prev->next_free = to_remove->next_free;
        }
}

    to_remove->next_free = free_stack;
    free_stack= to_remove;

}


void removeFromFreeStack(struct block *to_remove) {

  
   struct block *prev = NULL;
   struct block *current=free_stack;


    while (current != NULL && current != to_remove) {
        prev = current;
        current = current->next_free;
    }

    if (current == to_remove) {
        if (prev == NULL) {
            free_stack = to_remove->next_free;
        } else {
            prev->next_free = to_remove->next_free;
        }
     }

}








void merge(){

	struct block *current=heap_start;


	while (current && current->next) {
        	if (current->next->isFree && current->isFree) {
	   		if(isInFreeStack(current->next)){
				removeFromFreeStack(current->next);
	    		}
	    	if(heap_end==current->next){
			heap_end=current;
	    	}
                current->size += current->next->size;
                current->next = current->next->next;
                removeAndAdd(current);	
               }else{
			current=current->next;}
    	 }



}

void *kumalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }


    size_t total_size = sizeof(struct block) + size;
    total_size += sizeof(long) - (total_size % sizeof(long));

    struct block *header = NULL;


    struct block **prev_ptr = &free_stack;
    while (*prev_ptr) {
        if ((*prev_ptr)->size >= total_size && (*prev_ptr)->isFree) {
            header = *prev_ptr;
            *prev_ptr = header->next_free;
            break;
        }
        prev_ptr = &(*prev_ptr)->next_free;
    }
 
      if (header) {
        
         size_t remaining_size = header->size - total_size;
         if (remaining_size >sizeof(struct block)) {
            struct block *new_block = (struct block *)((char *)header + total_size);
            new_block->size = remaining_size;
            new_block->isFree = 1;
            new_block->next_free = free_stack;
            free_stack = new_block;
            header->size = total_size;
        }

        header->isFree = 0;
        return (void *)(header + 1);
    }



    void *block=sbrk(8192+total_size);
 
    if (block == (void *)-1) {
        return NULL; 
    }

   
    header = block;
    struct block *bb = (struct block *)((char *)header+total_size);
    header->size = total_size;
    bb->size=8192;
    header->isFree = 0;
    bb->isFree=1;
    header->next=bb;
    bb->next_free=free_stack;
    free_stack=bb;

    if (!heap_start) {
        heap_start = header;
	
    } else {
        heap_end->next = header;
    }

    heap_end = bb;
   

    return (void *)(header + 1);
}

 void *kucalloc(size_t nmemb, size_t size)
{
	size_t memory_size=nmemb*size;
	if(memory_size){
		void *ptr=kumalloc(memory_size);
		memset(ptr,0,memory_size);
		if(ptr){
			return ptr;
		}else{
			return NULL;
		}
	}
	return NULL; 
}

void kufree(void *ptr) {
    if (!ptr) {
        return;
    }

        struct block *block_to_free = (struct block *)ptr - 1;
        block_to_free->isFree = 1;

//	if(isMergable()){
//		merge();

//	}else{
       		block_to_free->next_free = free_stack;
        	free_stack = block_to_free;



  // }
}
void *kurealloc(void *ptr, size_t size) {
    if (!ptr) {
        return kumalloc(size);
    }

    if (size == 0) {

        kufree(ptr);
        return NULL;
    }

    struct block *header = (struct block *)ptr - 1;
    size_t old_size = header->size-sizeof(struct block);

    if (old_size >= size) {

        return ptr;
    }

    void *new_ptr = kumalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, old_size);
    kufree(ptr); 
    return new_ptr; 
}




/* Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */
#if 1
void *malloc(size_t size) { return kumalloc(size); }
void *calloc(size_t nmemb, size_t size) { return kucalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return kurealloc(ptr, size); }
void free(void *ptr) { kufree(ptr); }
#endif
