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

#include "HYPERBUSFBlockDevice.h"

#define HYPERBUS_SIZE    64*1024*1024

/*
|+-+-+-+-+-+-+-+-|
|                |
|   USER  APP    |  256K
|+-+-+-+-+-+-+-+-|  HYPERBUS_FILE_SYSTEM_ADDR_OFFSET
|                |
|  FILE SYSTEM   |  256K
|+-+-+-+-+-+-+-+-|
|                |
|      ...       |  256K
|+-+-+-+-+-+-+-+-|
|                |
|      ...       |  256K
|+-+-+-+-+-+-+-+-|
|                |
|      ...       |  256K
|+-+-+-+-+-+-+-+-|
|                |
|      ...       |  ...
|+-+-+-+-+-+-+-+-|
|                |
*/

#define HYPERBUS_FILE_SYSTEM_ADDR_OFFSET  256*1024

// Read/write/erase sizes
#define HYPERBUS_READ_SIZE  2
#define HYPERBUS_PROG_SIZE  2
#define HYPERBUS_SE_SIZE    256*1024
#define HYPERBUS_TIMEOUT    10000

// Status register
#define HYPERBUS_DEVICE_READY   0x80
#define HYPERBUS_ERASE_STATUS   0x20
#define HYPERBUS_PROGRAM_STATUS 0x10


HYPERBUSFBlockDevice::HYPERBUSFBlockDevice(PinName dq0, PinName dq1, PinName dq2, PinName dq3,
                                         PinName dq4, PinName dq5, PinName dq6, PinName dq7,
                                         PinName ck, PinName ckn, PinName rwds, PinName ssel0,
                                         PinName ssel1) :
    _hyperbus(dq0, dq1, dq2, dq3, dq4, dq5, dq6, dq7, ck, ckn, rwds, ssel0, ssel1),
    _size(HYPERBUS_SIZE)
{
    int latency = 0;

    /* Config memory maximum transfer data length for TX and RX*/
    _hyperbus.set_max_length(uHYPERBUS_Flash, 0x1ff, 1);
    _hyperbus.set_max_length(uHYPERBUS_Flash, 0x1ff, 1);

    /* Config memory access timing for TX and RX*/
    _hyperbus.set_timing(uHYPERBUS_Flash, 4, 4, 4, latency);
    _hyperbus.set_timing(uHYPERBUS_Flash, 4, 4, 4, latency);
}

int HYPERBUSFBlockDevice::init()
{
    /* Set VCR to 5 delay cycles */
    _hyperbus.write(0x555 << 1, 0xAA, uHYPERBUS_Mem_Access);
    _hyperbus.write(0x2AA << 1, 0x55, uHYPERBUS_Mem_Access);
    _hyperbus.write(0x555 << 1, 0x38, uHYPERBUS_Mem_Access);
    _hyperbus.write(0     << 1, 0x8e0b, uHYPERBUS_Mem_Access);

    return 0;
}

int HYPERBUSFBlockDevice::deinit()
{
    return 0;
}

int HYPERBUSFBlockDevice::_sync()
{
    for (int i = 0; i < HYPERBUS_TIMEOUT; i++) {
        /* Read status register */
        _hyperbus.write(0x555 << 1, 0x70, uHYPERBUS_Mem_Access);

        uint16_t status = _hyperbus.read(0, uHYPERBUS_Mem_Access);

        // Check Device Ready bit
        if (status & HYPERBUS_DEVICE_READY) {
            return 0;
        }

        wait_ms(1);
    }

    return BD_ERROR_DEVICE_ERROR;
}

int HYPERBUSFBlockDevice::_wren()
{
    return 0;
}

int HYPERBUSFBlockDevice::read(void *buffer, bd_addr_t addr, bd_size_t size)
{
    // Check the address and size fit onto the chip.
    MBED_ASSERT(is_valid_read(addr, size));

    _hyperbus.read_block(addr + HYPERBUS_FILE_SYSTEM_ADDR_OFFSET, (char*)buffer, size, uHYPERBUS_Mem_Access);

    return 0;
}

int HYPERBUSFBlockDevice::program(const void *buffer, bd_addr_t addr, bd_size_t size)
{
    // Check the address and size fit onto the chip.
    MBED_ASSERT(is_valid_program(addr, size));

    while (size > 0) {
        int err = _wren();
        if (err) {
            return err;
        }

        // Write up to 256*2 bytes a page
        // TODO handle unaligned programs
        uint32_t off = addr % 512;
        uint32_t chunk = (off + size < 512) ? size : (512-off);

        /* Command Sequence */
        _hyperbus.write(0x555 << 1, 0xAA, uHYPERBUS_Mem_Access);
        _hyperbus.write(0x2AA << 1, 0x55, uHYPERBUS_Mem_Access);
        _hyperbus.write(0x555 << 1, 0xA0, uHYPERBUS_Mem_Access);

        /* Word Program */
        _hyperbus.write_block(addr + HYPERBUS_FILE_SYSTEM_ADDR_OFFSET, (char *)buffer, chunk, uHYPERBUS_Mem_Access);

        buffer = static_cast<const uint8_t*>(buffer) + chunk;
        addr += chunk;
        size -= chunk;

        err = _sync();
        if (err) {
            return err;
        }
    }


    return 0;
}

int HYPERBUSFBlockDevice::erase(bd_addr_t addr, bd_size_t size)
{
    // Check the address and size fit onto the chip.
    MBED_ASSERT(is_valid_erase(addr, size));

    while (size > 0) {
        int err = _wren();
        if (err) {
            return err;
        }

        // Erase 256kbyte sectors
        // TODO support other erase sizes?
        uint32_t chunk = HYPERBUS_SE_SIZE;

        /* Erase sector */
        _hyperbus.write(0x555 << 1, 0xAA, uHYPERBUS_Mem_Access);
        _hyperbus.write(0x2AA << 1, 0x55, uHYPERBUS_Mem_Access);
        _hyperbus.write(0x555 << 1, 0x80, uHYPERBUS_Mem_Access);
        _hyperbus.write(0x555 << 1, 0xAA, uHYPERBUS_Mem_Access);
        _hyperbus.write(0x2AA << 1, 0x55, uHYPERBUS_Mem_Access);

        _hyperbus.write(addr + HYPERBUS_FILE_SYSTEM_ADDR_OFFSET, 0x30, uHYPERBUS_Mem_Access);

        addr += chunk;
        size -= chunk;

        err = _sync();
        if (err) {
            return err;
        }
    }

    return 0;
}

bd_size_t HYPERBUSFBlockDevice::get_read_size() const
{
    return HYPERBUS_READ_SIZE;
}

bd_size_t HYPERBUSFBlockDevice::get_program_size() const
{
    return HYPERBUS_PROG_SIZE;
}

bd_size_t HYPERBUSFBlockDevice::get_erase_size() const
{
    return HYPERBUS_SE_SIZE;
}

bd_size_t HYPERBUSFBlockDevice::size() const
{
    return _size;
}

int HYPERBUSFBlockDevice::get_erase_value() const
{
    return 0xFF;
}
