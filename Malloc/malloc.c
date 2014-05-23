#include "malloc.h"
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#define ALIGNMENT 8


int takeCount = 0;

struct Header {
	struct Header *next;
	size_t size;
} typedef Header;

Header *freeList = NULL;

void printList() {
	int i = 0;
	printf("THIS IS HOW THE LIST LOOKS RIGHT NOW\n");
	for (;freeList != NULL; freeList = freeList->next) {
		printf("%d, size: %ld, address: %p\n", i, freeList->size, freeList);
		i++;
	}
}

long closestAlignedAddress(long address) {
    long nextAddress = address - (address % ALIGNMENT);
    if (nextAddress < address) {
        nextAddress += ALIGNMENT;
    }
    return nextAddress;
}

Header * getNextHeader(long start, long end) {
	long nextPosition = closestAlignedAddress(start);
	long emptySpace = end - nextPosition - sizeof(Header);

    if (emptySpace <= 0) return NULL;
    Header *header = (Header *)nextPosition;
    header->size = emptySpace;
    header->next = NULL;
    return header;
}

void insertIntoFreeList(Header *header) {
    assert(header != NULL);
    assert(header->size);
    header->next = NULL;
    if (!header->size) {
        
        
    }
    Header *current = freeList;
    Header *prev = NULL;
    while (current && current < header) {
        prev = current;
        current = current->next;
    }
    
    if (prev == header) {
        
        printf("HM");
        
    }
    
    if (prev) {
        prev->next = header;
    } else {
        freeList = header;
    }
    header->next = current;
}

Header * getEmptyHeader(size_t size) {
    takeCount++;
	Header *prev = NULL;
	Header *current = freeList;
	while (current) {
		if (current->size >= size) {
            if (!prev) {
                freeList = current->next;
            } else {
                prev->next = current->next;
            }
            long start = (long)current + size + sizeof(Header);
            long end = (long)current + current->size + sizeof(Header);
            
            Header * nextHeader = getNextHeader(start, end);
            
            
            //printf("%p, %p\n", current, nextHeader);
            
            if (nextHeader) {
                assert(size < current->size);
                assert((long)current + size + sizeof(Header) <= (long)nextHeader);
                assert(nextHeader > current);
                assert((long)nextHeader->size < end - start);
                
                insertIntoFreeList(nextHeader);
            }
            current->size = size;
			return current;
		}
		prev = current;
        current = current->next;
	}
	return NULL;
}


void allocateMoreSpace(size_t size) {
	long pageSize = sysconf(_SC_PAGESIZE);
	long usedSize = (int)size + sizeof(Header);
	long pages = usedSize / pageSize + 1;
	long totalSize = pages * pageSize;
	Header * header = mmap(NULL, totalSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    assert(header);
	header->size = size;
	insertIntoFreeList(header);
    long start = (long)header + usedSize;
    long end = (long)header + totalSize;
    Header *nextHeader = getNextHeader(start, end);
    if (nextHeader) {
        insertIntoFreeList(nextHeader);
    } else {
        //header->size = totalSize - sizeof(Header);
    }
}

void * malloc(size_t size) {
    if (size <= 0) return NULL;
	Header * emptyHeader = getEmptyHeader(size);
	if (!emptyHeader) {
		allocateMoreSpace(size);
        emptyHeader = getEmptyHeader(size);
        assert(emptyHeader);
	}
    return (void *)(emptyHeader + 1);
}

void * realloc(void *p, size_t size) {
    if (size <= 0) return NULL;
    if (!p) return malloc(size);
    Header *header = (Header *)p - 1;
    
    void *data = malloc(size);
    long minSize = size < header->size ? size : header->size;
    long i;
    for (i = 0; i < minSize; i++) {
        ((char *)data)[i] = ((char *)p)[i];
    }
    //free(p);
    return data;
}

void mergeList() {
    if (!freeList) return;
    Header *current = freeList;
    while (current && current->next) {
        if ((long)current + current->size + sizeof(Header) == (long)current->next) {
            current->size += sizeof(Header) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }
}

void free(void *p) {
    if (!p) return;
    Header *header = (Header *)p - 1;
    assert(header);
    assert(header->size);
    insertIntoFreeList(header);
    //mergeList();
}