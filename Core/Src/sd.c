#include "sd.h"
#include "spi.h"
#include <stdint.h>
#include <stdio.h>

static int bit_len(uint64_t x) {
  int n = 0;
  while (x) { n++; x >>= 1; }
  return n;
}

static uint64_t crc(uint64_t msg) {
  const uint64_t g = 137;
  const int n = bit_len(g);
  msg <<= n - 1;
  while (bit_len(msg) >= n)
      msg ^= g << (bit_len(msg) - n);
  return msg;
}

static uint8_t spi_xfer(uint8_t tx) {
  uint8_t rx;
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 100);
  return rx;
}

static uint8_t sd_cmd(uint8_t index, uint32_t argument) {
  uint64_t frame = 1;
  frame |= (uint64_t)1 << 46;
  frame |= (uint64_t)index << 40;
  frame |= (uint64_t)argument << 8;
  frame |= crc(frame >> 8) << 1;

  //fix endianness
  uint8_t buf[6];
  for (int i = 0; i < 6; i++)
    buf[i] = (uint8_t)(frame >> (40 - 8 * i));

  HAL_SPI_Transmit(&hspi1, buf, 6, 100);

  uint8_t r1 = 0xFF;
  for (int i = 0; i < 8 && r1 == 0xFF; i++)
    r1 = spi_xfer(0xFF);
  return r1;
}

static uint32_t sd_read32(void) {
  uint32_t v = 0;
  for (int i = 0; i < 4; i++)
    v = (v << 8) | spi_xfer(0xFF);
  return v;
}

static void print_r1(uint8_t r1) {
  printf("%02X\n", r1);
  if (r1 >> 0 & 1) printf("In Idle State\n");
  if (r1 >> 1 & 1) printf("Erase State\n");
  if (r1 >> 2 & 1) printf("Illegal Command\n");
  if (r1 >> 3 & 1) printf("Com CRC Error\n");
  if (r1 >> 4 & 1) printf("Erase Sequence Error\n");
  if (r1 >> 5 & 1) printf("Address Error\n");
  if (r1 >> 6 & 1) printf("Parameter Error\n");
}

static void print_r7(uint32_t r7) {
  printf("%08lX\n", (unsigned long)r7);

  if (r7 >> 13 & 1) printf("PCIe 1.2V Support Accepted\n");

  uint8_t volt = (r7 >> 8) & 0x0F;
  if (volt == 0x0)      printf("Voltage Not Defined\n");
  else if (volt == 0x1) printf("2.7-3.6V\n");
  else if (volt == 0x2) printf("Reserved for Low Voltage Range\n");
  else                  printf("Unknown voltage: %X\n", volt);
}

int sd_init(void) {
  uint8_t r1;

  // power up: 80 clocks (10 bytes of 0xFF) with CS high
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  for (int i = 0; i < 10; i++) spi_xfer(0xFF);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  r1 = sd_cmd(0, 0);
  printf("CMD0 Response:\n");
  print_r1(r1);
  printf("\n");

  r1 = sd_cmd(8, 0x1AA);
  printf("CMD8 Response:\n");
  print_r1(r1);
  print_r7(sd_read32());
  printf("\n");

  printf("Initializing...\n");
  do {
    sd_cmd(55, 0);
    r1 = sd_cmd(41, 1 << 30);
  } while (r1 == 0x01);

  sd_cmd(58, 0);
  uint32_t r3 = sd_read32();
  if (r3 >> 30 & 1) printf("SDHC/SDXC Ready...\n");
  else              printf("SDSC Ready...\n");
  printf("\n");

  return 0;
}

int sd_write(uint32_t address, const void* data, uint16_t length) {
  uint8_t r1;
  r1 = sd_cmd(24, address);
  if (r1 != 0x00) {
    printf("ERROR: Failed to Enter Write Mode\n");
    return 1;
  } else printf("Successfully Entered Write Mode...\n");

  //send start block token
  spi_xfer(0xFE);

  for (uint16_t i = 0; i < 512; i++) {
    if (i < length)
      spi_xfer(((uint8_t*)data)[i]);
    else
      spi_xfer(0x00);
  }

  //dummy crc
  spi_xfer(0xFF);
  spi_xfer(0xFF);
  
  uint8_t response = spi_xfer(0xFF);
  if ((response & 0x1F) != 0x05) {
    return 1;
  } else printf("Successfully Wrote Block...\n");

  //wait until card is ready
  while (spi_xfer(0xFF) == 0x00);

  return 0;
}

int sd_read(uint32_t address, uint8_t* buffer, uint16_t buffer_size) {
    uint8_t r1;
    r1 = sd_cmd(17, address);
    if (r1 != 0x00) {
      printf("\nERROR: Failed to Enter Read Mode\n");
      return 1;
    } else printf("\nSuccessfully Entered Read Mode...\n");

    // keep receiving bytes until the data token
    while (spi_xfer(0xFF) != 0xFE);

    for (uint16_t i = 0; i < 512; i++) {
      uint8_t r_byte = spi_xfer(0xFF);

      if (i < buffer_size) {
        buffer[i] = r_byte;
      }
    }

    //dummy crc
    spi_xfer(0xFF);
    spi_xfer(0xFF);

    //dummy byte
    spi_xfer(0xFF);

    return 0;
}

void hexdump(const uint8_t *buffer, uint32_t length, uint32_t base) {
  for (uint32_t offset = 0; offset < length; offset += 16) {
    printf("%08lX  ", (unsigned long)(base + offset));

    for (int i = 0; i < 16; i++) {
      if (offset + i < length) printf("%02X ", buffer[offset + i]);
      else               printf("   ");
      if (i == 7) printf(" ");
    }

    printf(" |");
    for (int i = 0; i < 16 && offset + i < length; i++) {
      uint8_t c = buffer[offset + i];
      printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
    }
    printf("|\n");
  }
}

int sd_dump_block(uint32_t block) {
  static uint8_t buf[512];

  if (sd_read(block, buf, sizeof(buf)) != 0) return 1;

  hexdump(buf, sizeof(buf), block * 512);
  return 0;
}
