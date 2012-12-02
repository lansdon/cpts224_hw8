#include "cma.h"
#include <stdlib.h>

#define MSIZE 1024*16
unsigned char mem[MSIZE];

int main(int argc, char * argv[]) {

	int i=1;
	int start_size=10;

	if(argv[1]) {	
		start_size = atoi(argv[1]);
	} 

	class_memory(malloc(start_size),start_size); // give it memory allocator

	while(1) {

		if(! (char *)class_malloc(i) ) {  //allocate from classmem library
			printf("Error! %d is too much!\n", i);
			i >>= 1;
			printf("Last successful memory size: %d\n", i);
			break;
		} else {
			printf("Size=%d\n",i);
			i <<= 1;
		}			
	}
}
