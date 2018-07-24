/* mbed Microcontroller Library
 * Copyright (c) 2018 2018 GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MBED_HYPERBUS_BLOCK_DEVICE_H
#define MBED_HYPERBUS_BLOCK_DEVICE_H

#include <mbed.h>
#include "BlockDevice.h"


/** BlockDevice for HYPERBUS based flash devices
 *  such as the MX25R or SST26F016B
 *
 *  @code
 *  // Here's an example using the MX25R HYPERBUS flash device on the K82F
 *  #include "mbed.h"
 *  #include "HYPERBUSFBlockDevice.h"
 *
 *  // Create flash device on HYPERBUS bus with GAP8
 *  HYPERBUSBlockDevice hyperbusf(HYPERBUS_DQ0, HYPERBUS_DQ1, HYPERBUS_DQ2, HYPERBUS_DQ3,
 *                                HYPERBUS_DQ4, HYPERBUS_DQ5, HYPERBUS_DQ6, HYPERBUS_DQ7,
 *                                HYPERBUS_CLK, HYPERBUS_CLKN, HYPERBUS_RWDS, HYPERBUS_CSN0, HYPERBUS_CSN1);
 *
 *  int main() {
 *      printf("hyperbusf test\n");
 *
 *      // Initialize the HYPERBUS flash device and print the memory layout
 *      hyperbusf.init();
 *      printf("hyperbusf size: %llu\n",         hyperbusf.size());
 *      printf("hyperbusf read size: %llu\n",    hyperbusf.get_read_size());
 *      printf("hyperbusf program size: %llu\n", hyperbusf.get_program_size());
 *      printf("hyperbusf erase size: %llu\n",   hyperbusf.get_erase_size());
 *
 *      // Write "Hello World!" to the first block
 *      char *buffer = (char*)malloc(hyperbusf.get_erase_size());
 *      sprintf(buffer, "Hello World!\n");
 *      hyperbusf.erase(0, hyperbusf.get_erase_size());
 *      hyperbusf.program(buffer, 0, hyperbusf.get_erase_size());
 *
 *      // Read back what was stored
 *      hyperbusf.read(buffer, 0, hyperbusf.get_erase_size());
 *      printf("%s", buffer);
 *
 *      // Deinitialize the device
 *      hyperbusf.deinit();
 *      free(buffer);
 *  }
 *  @endcode
 */
class HYPERBUSFBlockDevice : public BlockDevice {
public:
    /** Creates a HYPERBUSFBlockDevice on a HYPERBUS bus specified by pins
     *
     * @param  ck   The pin to use for CLK
     * @param  ckn  The pin to use for CLK NEG
     * @param  rwds The pin to use for RWDS
     * @param  dq0  The pin to use for DQ0
     * @param  dq1  The pin to use for DQ1
     * @param  dq2  The pin to use for DQ2
     * @param  dq3  The pin to use for DQ3
     * @param  dq4  The pin to use for DQ4
     * @param  dq5  The pin to use for DQ5
     * @param  dq6  The pin to use for DQ6
     * @param  dq7  The pin to use for DQ7
     * @param  ssel0 The pin to use for CSN0
     * @param  ssel1 The pin to use for CSN0
     */
    HYPERBUSFBlockDevice(PinName dq0, PinName dq1, PinName dq2, PinName dq3,
                         PinName dq4, PinName dq5, PinName dq6, PinName dq7,
                         PinName ck, PinName ckn, PinName rwds, PinName ssel0,
                         PinName ssel1=NC);

    /** Initialize a block device
     *
     *  @return         0 on success or a negative error code on failure
     */
    virtual int init();

    /** Deinitialize a block device
     *
     *  @return         0 on success or a negative error code on failure
     */
    virtual int deinit();

    /** Read blocks from a block device
     *
     *  @param buffer   Buffer to write blocks to
     *  @param addr     Address of block to begin reading from
     *  @param size     Size to read in bytes, must be a multiple of read block size
     *  @return         0 on success, negative error code on failure
     */
    virtual int read(void *buffer, bd_addr_t addr, bd_size_t size);

    /** Program blocks to a block device
     *
     *  The blocks must have been erased prior to being programmed
     *
     *  @param buffer   Buffer of data to write to blocks
     *  @param addr     Address of block to begin writing to
     *  @param size     Size to write in bytes, must be a multiple of program block size
     *  @return         0 on success, negative error code on failure
     */
    virtual int program(const void *buffer, bd_addr_t addr, bd_size_t size);

    /** Erase blocks on a block device
     *
     *  The state of an erased block is undefined until it has been programmed
     *
     *  @param addr     Address of block to begin erasing
     *  @param size     Size to erase in bytes, must be a multiple of erase block size
     *  @return         0 on success, negative error code on failure
     */
    virtual int erase(bd_addr_t addr, bd_size_t size);

    /** Get the size of a readable block
     *
     *  @return         Size of a readable block in bytes
     */
    virtual bd_size_t get_read_size() const;

    /** Get the size of a programable block
     *
     *  @return         Size of a programable block in bytes
     *  @note Must be a multiple of the read size
     */
    virtual bd_size_t get_program_size() const;

    /** Get the size of a eraseable block
     *
     *  @return         Size of a eraseable block in bytes
     *  @note Must be a multiple of the program size
     */
    virtual bd_size_t get_erase_size() const;

    /** Get the value of storage when erased
     *
     *  If get_erase_value returns a non-negative byte value, the underlying
     *  storage is set to that value when erased, and storage containing
     *  that value can be programmed without another erase.
     *
     *  @return         The value of storage when erased, or -1 if you can't
     *                  rely on the value of erased storage
     */
    virtual int get_erase_value() const;

    /** Get the total size of the underlying device
     *
     *  @return         Size of the underlying device in bytes
     */
    virtual bd_size_t size() const;

private:
    // Master side hardware
    HYPERBUS _hyperbus;
    /* DigitalOut _cs; */

    // Device configuration discovered through sfdp
    bd_size_t _size;

    // Internal functions
    int _wren();
    int _sync();
};


#endif  /* MBED_HYPERBUS_BLOCK_DEVICE_H */
