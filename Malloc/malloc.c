#include "malloc.h"
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

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

Header * getNextHeader(long start, long end) {

    
	long nextPosition = start - (start % ALIGNMENT);
    if (nextPosition < start) {
        nextPosition += ALIGNMENT;
    }
	long emptySpace = end - nextPosition - sizeof(Header);
    if (emptySpace <= 0) return NULL;
    
    if (emptySpace > 80 * 1000) {
        printf("HMM!");
    }
    Header *header = (Header *)nextPosition;
    header->size = emptySpace;
    header->next = NULL;
    return header;
}

void insertIntoFreeList(Header *header) {
    if (header->size > 80 * 1000) {
        printf("THE HELL!");
    }
    Header *current = freeList;
    Header *prev = NULL;
    while (current && current < header) {
        prev = current;
        current = current->next;
    }
    if (prev) {
        prev->next = header;
    } else {
        freeList = header;
    }
    header->next = current;
}

void * takeEmptySpace(size_t size) {
    takeCount++;
	Header *prev = NULL;
	Header *current = NULL;
	for (current = freeList; current != NULL; current = current->next) {
		if (current->size >= size) {
            long start = (long)current + size + sizeof(Header);
            long end = (long)current + current->size + sizeof(Header);
            Header * nextHeader = getNextHeader(start, end);
            if (nextHeader && nextHeader->size > 80 * 1000) {
                printf("THE FUCK!\n");
            }
            if (nextHeader) {
                if (!prev) {
                    freeList = nextHeader;
                } else {
                    prev->next = nextHeader;
                    nextHeader->next = current->next;
                }
            } else {
                if (!prev) {
                    freeList = current->next;
                } else {
                    prev->next = current->next;
                }
            }
			return current+1;
		}
		prev = current;
	}
	return NULL;
}


void allocateMoreSpace(size_t size) {
	long pageSize = sysconf(_SC_PAGESIZE);
	long usedSize = (int)size + sizeof(Header);
	long pages = usedSize / pageSize + 1;
	long totalSize = pages * pageSize;
	Header * header = mmap(NULL, totalSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    
    if (!header) {
        printf("NO HEADER\n");
    }
    
    if (size > 8 * 10000) {
        printf("LARGE MEMORY\n");
    }
    
	header->size = size;
    header->next = NULL;
    
	insertIntoFreeList(header);
    
    long start = (long)header + usedSize;
    long end = (long)header + totalSize;
    Header *nextHeader = getNextHeader(start, end);
    
    
    if (nextHeader->size > 80 * 1000) {
        printf("WTF\n");
    }
    
    if (nextHeader) {
        insertIntoFreeList(nextHeader);
    }
}

void * malloc(size_t size) {
    if (size <= 0) return NULL;
	void * emptySpace = takeEmptySpace(size);
	if (!emptySpace) {
		allocateMoreSpace(size);
        emptySpace = takeEmptySpace(size);
		return emptySpace;
	} else {
		return emptySpace;
	}
}

void * realloc(void *p, size_t size) {
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
    if (header == freeList) return;
    header->next = freeList;
    if (header->size > 80 * 1000) {
        printf("FUCKED ME UP");
    }
    freeList = header;
    mergeList();
}