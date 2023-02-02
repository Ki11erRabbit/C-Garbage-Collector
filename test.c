//
// Created by ki11errabbit on 1/26/23.
//
#include <stdio.h>
#include <unistd.h>

#include "gc.h"

void allocateMemory(){
    char *a = malloc(20);
    char *b = malloc(20);
    char *c = malloc(20);
    char *d = malloc(20);
    updateStackPointer();
}
void recurse5Times(int counter) {
    if (counter == 5) {
        return;
    }
    recurse5Times(counter + 1);
}

int main(){

    allocateMemory();
    updateStackPointer();

    for (int i =0; i < 1000000000; i++){
        recurse5Times(0);
    }
    //sleep(10);

    return 0;
}