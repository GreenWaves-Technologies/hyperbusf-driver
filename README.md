# HYPERBUS Flash Driver

Block device driver for NOR based HYPERBUS flash devices [CYPRESS](http://www.cypress.com/products/hyperbus-memory).

NOR based HYPERBUS flash supports byte-sized read and writes, with an erase size of around 256kbytes. An erase sets a block to all 1s, with successive writes clearing set bits.

More info on NOR flash can be found on wikipedia:
https://en.wikipedia.org/wiki/Flash_memory#NOR_memories

``` cpp
    // Here's an example using the S71KS512 HYPERBUS flash device on the GAP8
    #include "mbed.h"
    #include "HYPERBUSFBlockDevice.h"

    // Create flash device on HYPERBUS bus with GAP8
    HYPERBUSBlockDevice hyperbusf(HYPERBUS_DQ0, HYPERBUS_DQ1, HYPERBUS_DQ2, HYPERBUS_DQ3,
                           HYPERBUS_DQ4, HYPERBUS_DQ5, HYPERBUS_DQ6, HYPERBUS_DQ7,
                           HYPERBUS_CLK, HYPERBUS_CLKN, HYPERBUS_RWDS, HYPERBUS_CSN0, HYPERBUS_CSN1);

    int main() {
        printf("hyperbusf test\n");

        // Initialize the HYPERBUS flash device and print the memory layout
        hyperbusf.init();
        printf("hyperbusf size: %llu\n",         hyperbusf.size());
        printf("hyperbusf read size: %llu\n",    hyperbusf.get_read_size());
        printf("hyperbusf program size: %llu\n", hyperbusf.get_program_size());
        printf("hyperbusf erase size: %llu\n",   hyperbusf.get_erase_size());

        // Write "Hello World!" to the first block
        char *buffer = (char*)malloc(hyperbusf.get_erase_size());
        sprintf(buffer, "Hello World!\n");
        hyperbusf.erase(0, hyperbusf.get_erase_size());
        hyperbusf.program(buffer, 0, hyperbusf.get_erase_size());

        // Read back what was stored
        hyperbusf.read(buffer, 0, hyperbusf.get_erase_size());
        printf("%s", buffer);

        // Deinitialize the device
        hyperbusf.deinit();
        free(buffer);
    }
```

