#include "dac.h"
#include "i2s.h"
#include "stm32f446xx.h"
#include <stdint.h>
static int start = 3;
static int total = 10;

void play_audio(uint16_t *audio, int len, uint8_t *playing) {
  HAL_I2S_DMAStop(&hi2s2);
  HAL_I2S_Transmit_DMA(&hi2s2, audio, len);
}

