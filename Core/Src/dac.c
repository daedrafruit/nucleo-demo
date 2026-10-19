#include "dac.h"
#include "i2s.h"
#include <stdint.h>

static int start = 3;
static int total = 10;

void play_audio(uint16_t *audio, int len, uint8_t *playing) {
  uint8_t a_on = 0;
  uint8_t b_on = 0;
  for (int i = 0; i < 5; i++) {
    a_on |= !(HAL_GPIO_ReadPin(GPIOC, 1 << i));
  }
  for (int i = 5; i < 9; i++) {
    b_on |= !(HAL_GPIO_ReadPin(GPIOC, 1 << i));
  }
  if (!a_on && !b_on) {
    *playing = 0;
    return;
  }
  if (*playing) 
    return; 
  *playing = 1;

  if (a_on) {
    HAL_I2S_DMAStop(&hi2s2);
    HAL_I2S_Transmit_DMA(&hi2s2, audio, len);
  } 
  else if (b_on) {
    HAL_I2S_DMAStop(&hi2s2);
    HAL_I2S_Transmit_DMA(&hi2s2, audio + (len * start)/total, len * (total - start)/total);
  }
}
