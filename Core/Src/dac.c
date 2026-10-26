#include "i2s.h"
#include "sd.h"
#include <stdint.h>
#include <string.h>
//0.03
//0.47
//0.91
//1.35
//1.79
//6.3
//end = 6.62

#define BLOCK_SIZE 512
#define HALF_LEN (BLOCK_SIZE / 2)
#define CLIP_BLOCKS 827

static uint16_t buf[BLOCK_SIZE];
static volatile int8_t ready = -1;
static uint32_t block, end;
static uint8_t playing = 0;

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) { ready = 0; }
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) { ready = 1; }
void audio_stop() { playing = 0; }

void audio_init(void) {
  memset(buf, 0, sizeof buf);
  HAL_I2S_Transmit_DMA(&hi2s2, buf, BLOCK_SIZE);
}
static const uint16_t slice_start[] = { 3, 58, 113, 168, 223, 787 };
#define NUM_SLICES (sizeof slice_start / sizeof slice_start[0])

void play_audio(uint8_t button) {
  if (button >= NUM_SLICES) return;
  block = slice_start[button];
  end = slice_start[button] + 55;
  playing = 1;
}

void audio_set_rate(uint32_t freq) {
  HAL_I2S_DMAStop(&hi2s2);
  hi2s2.Init.AudioFreq = freq;
  HAL_I2S_Init(&hi2s2);
  ready = -1;
  HAL_I2S_Transmit_DMA(&hi2s2, buf, BLOCK_SIZE);
}

void audio_update() {
  if (ready < 0) return;
  uint8_t *dst = (uint8_t *)&buf[ready * HALF_LEN];
  ready = -1;
  memset(dst, 0, BLOCK_SIZE);
  if (!playing) return;
  for (int i = 0; i < 1 && block < end; i++)
    sd_read(block++, dst + i * BLOCK_SIZE, BLOCK_SIZE);
  if (block >= end) playing = 0;
}
