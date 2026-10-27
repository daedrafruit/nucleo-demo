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
#define CLIP_BLOCKS 827
// ~124.925 blocks per second
// 8th note = ~55 block
#define BLOCKS_PER_SECOND 125
#define EIGHTH_NOTE 55

#define BLOCK_SIZE 512
#define HALF_LEN (BLOCK_SIZE / 2)

static uint16_t buf[BLOCK_SIZE];
static volatile uint32_t block, end;
static volatile uint8_t playing = 0;
static const uint16_t slice_start[] = { 3, 58, 113, 168, 223, 787 };
#define NUM_SLICES (sizeof slice_start / sizeof slice_start[0])

static void fill_half(int half) {
  uint8_t *dst = (uint8_t *)&buf[half * HALF_LEN];
  memset(dst, 0, BLOCK_SIZE);
  if (!playing) return;
  if (block < end)
    sd_read(block++, dst, BLOCK_SIZE);
  if (block >= end) playing = 0;
}

void audio_stop() { playing = 0; }

void audio_init(void) {
  memset(buf, 0, sizeof buf);
  HAL_I2S_Transmit_DMA(&hi2s2, buf, BLOCK_SIZE);
}

void play_audio(uint8_t button) {
  if (button >= NUM_SLICES) {
    audio_stop();
    return;
  }
  __disable_irq();
  block = slice_start[button];
  end = slice_start[button] + EIGHTH_NOTE;
  playing = 1;
  __enable_irq();
}

void audio_set_rate(uint32_t freq) {
  HAL_I2S_DMAStop(&hi2s2);
  hi2s2.Init.AudioFreq = freq;
  HAL_I2S_Init(&hi2s2);
  HAL_I2S_Transmit_DMA(&hi2s2, buf, BLOCK_SIZE);
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) { fill_half(0); }
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)     { fill_half(1); }
