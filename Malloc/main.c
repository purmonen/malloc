//#include "malloc.h"
#include <stdlib.h>
#include <stdio.h>

int main() {
    int size = 100;
    int i;
    char ** list = (char **)malloc(size*sizeof(char *));
    //void *p = malloc(1024 * 20); /* Se till att det finns fritt utrymme först i free-listan */
    
    for (i = 0; i<size; i++) list[i] = (char *)malloc(1); /* allokera massa små block */
    for (i = 1; i < size; i += 2) free(list[i]); /* Fria vart annat */
    
    //free(p);
    for (i=0;i<10000000;i++) free(malloc(10));/*Testet som faktiskt tar tid */
    
    printf("THE HELL");
}