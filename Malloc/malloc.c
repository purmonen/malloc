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

void removeFromFreeList(Header *header) {
    Header *current = freeList;
    Header *prev = NULL;
    while (current != header) {
        prev = current;
        current = current->next;
    }
    if (!current) return;
    if (prev) {
        prev->next = current->next;
    } else {
        freeList = current->next;
    }
}

void addRemainingHeader(Header *header, size_t size) {
    long start = (long)header + size + sizeof(Header);
    long end = (long)header + header->size + sizeof(Header);
    Header * nextHeader = getNextHeader(start, end);
    if (nextHeader) {
        insertIntoFreeList(nextHeader);
    }
}

Header * firstFitHeader(size_t size) {
    takeCount++;
	Header *current = freeList;
	while (current) {
		if (current->size >= size) {
            current->size = size;
            removeFromFreeList(current);
            addRemainingHeader(current, size);
			return current;
		}
        current = current->next;
	}
	return NULL;
}

Header * bestFitHeader(size_t size) {
	Header *current = freeList;
    Header *bestHeader = NULL;    
	while (current) {
		if (current->size >= size) {
            if (!bestHeader || (bestHeader->size - size) > (current->size - size)) {
                bestHeader = current;
            }
        }
        current = current->next;
    }
    if (!bestHeader) return NULL;
    bestHeader->size = size;
    removeFromFreeList(bestHeader);
    addRemainingHeader(bestHeader, size);
    return bestHeader;
}

Header * getEmptyHeader(size_t size) {
    return bestFitHeader(size);
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
        header->size = totalSize - sizeof(Header);
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
    free(p);
    return data;
}

void mergeList() {
    if (!freeList) return;
    Header *current = freeList;
    while (current && current->next) {
        if ((long)current + current->size + sizeof(Header) == (long)current->next) {
            //printf("MERGED LIST\n");
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
    mergeList();
}