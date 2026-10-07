#pragma once

#include <stdint.h>
int sd_init();
int sd_write(uint32_t address, const void* data, uint16_t length);
int sd_read(uint32_t address, uint8_t* buffer, uint16_t buffer_size);
int sd_dump_block(uint32_t block);
