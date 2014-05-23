#include "malloc.h"
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#define ALIGNMENT 8

struct Header {
	struct Header *next;
	size_t size;
} typedef Header;

Header *freeList = NULL;


static void * __endHeap = 0;

void * endHeap(void)
{
    if(__endHeap == 0) __endHeap = sbrk(0);
    return __endHeap;
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

void insertIntoList(Header **list, Header *header) {
    header->next = NULL;
    Header *current = *list;
    Header *prev = NULL;
    while (current && current < header) {
        prev = current;
        current = current->next;
    }
    if (prev) {
        prev->next = header;
    } else {
        *list = header;
    }
    header->next = current;
}

void removeFromList(Header **list, Header *header) {
    Header *current = *list;
    Header *prev = NULL;
    while (current != header) {
        prev = current;
        current = current->next;
    }
    if (!current) return;
    if (prev) {
        prev->next = current->next;
    } else {
        *list = current->next;
    }
}

void addRemainingHeaderToList(Header **list, Header *header, size_t size) {
    long start = (long)header + size + sizeof(Header);
    long end = (long)header + header->size + sizeof(Header);
    Header * nextHeader = getNextHeader(start, end);
    if (nextHeader) {
        insertIntoList(list, nextHeader);
    }
}

Header * firstFitHeaderFromList(Header **list, size_t size) {
	Header *current = *list;
	while (current) {
		if (current->size >= size) {
            current->size = size;
            removeFromList(list, current);
            addRemainingHeaderToList(list, current, size);
			return current;
		}
        current = current->next;
	}
	return NULL;
}

Header * bestFitHeaderFromList(Header **list, size_t size) {
	Header *current = *list;
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
    removeFromList(list, bestHeader);
    addRemainingHeaderToList(list, bestHeader, size);
    return bestHeader;
}

Header * getEmptyHeaderFromList(Header **list, size_t size) {
    return bestFitHeaderFromList(list, size);
}

void allocateMoreSpace(size_t size) {
	long pageSize = sysconf(_SC_PAGESIZE);
	long usedSize = (int)size + sizeof(Header);
	long pages = (usedSize - 1) / pageSize + 1;
	long totalSize = pages * pageSize;
	Header * header = mmap(NULL, totalSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (!header) return;
	header->size = size;
	insertIntoList(&freeList, header);
    long start = (long)header + usedSize;
    long end = (long)header + totalSize;
    Header *nextHeader = getNextHeader(start, end);
    if (nextHeader) {
        insertIntoList(&freeList, nextHeader);
    } else {
        header->size = totalSize - sizeof(Header);
    }
}

void * malloc(size_t size) {
    if (size <= 0) return NULL;
	Header * emptyHeader = getEmptyHeaderFromList(&freeList, size);
	if (!emptyHeader) {
		allocateMoreSpace(size);
        emptyHeader = getEmptyHeaderFromList(&freeList, size);
        if (!emptyHeader) return NULL;
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

void mergeList(Header **list) {
    if (!*list) return;
    Header *current = *list;
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
    insertIntoList(&freeList, header);
    mergeList(&freeList);
}