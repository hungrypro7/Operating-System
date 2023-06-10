#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 4096
#define ADDRESS_SPACE_BITS 32

void calculatePageOffset(unsigned int memoryAddress, unsigned int pageSize, unsigned int *pageNumber, unsigned int *offset) {
    *pageNumber = memoryAddress / pageSize;
    *offset = memoryAddress % pageSize;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./addresses <memory_address>\n");
        return 1;
    }

    unsigned int memoryAddress = atoi(argv[1]);
    unsigned int pageNumber, offset;

    // Calculate page number and offset
    calculatePageOffset(memoryAddress, PAGE_SIZE, &pageNumber, &offset);

    // Output the results
    printf("The address %u contains:\n", memoryAddress);
    printf("page number = %u\n", pageNumber);
    printf("offset = %u\n", offset);

    return 0;
}
