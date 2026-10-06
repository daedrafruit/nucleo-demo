#include "sd.h"
#include "spi.h"
#include <stdio.h>

static const uint8_t CMD0[]   = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
static const uint8_t CMD8[]   = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
static const uint8_t CMD55[]  = {0x77, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint8_t ACMD41[] = {0x69, 0x40, 0x00, 0x00, 0x00, 0x01};
static const uint8_t CMD58[]  = {0x7A, 0x00, 0x00, 0x00, 0x00, 0x01};

static uint8_t spi_xfer(uint8_t tx) {
  uint8_t rx;
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 100);
  return rx;
}

static uint8_t sd_cmd(const uint8_t *cmd) {
  HAL_SPI_Transmit(&hspi1, (uint8_t *)cmd, 6, 100);

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

  r1 = sd_cmd(CMD0);
  printf("CMD0 Response:\n");
  print_r1(r1);
  printf("\n");

  r1 = sd_cmd(CMD8);
  printf("CMD8 Response:\n");
  print_r1(r1);
  print_r7(sd_read32());
  printf("\n");

  printf("Initializing...\n");
  do {
    sd_cmd(CMD55);
    r1 = sd_cmd(ACMD41);
  } while (r1 == 0x01);

  sd_cmd(CMD58);
  uint32_t r3 = sd_read32();
  if (r3 >> 30 & 1) printf("SDHC/SDXC Ready...\n");
  else              printf("SDSC Ready...\n");

  return 0;
}
