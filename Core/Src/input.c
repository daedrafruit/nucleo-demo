#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "test_audio.h"
#include "gpio.h"
#include "dac.h"

volatile uint16_t input_buffer[32];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

// SOURCE: https://quant.engineering/ring-buffers-101.html
uint8_t enqueue(int x) {
    int next = (head + 1) & (32 - 1);
    if (next == tail) return 0;
    input_buffer[head] = x;
    head = next;
    return 1;
}

uint8_t dequeue(uint16_t *x) {
    if (head == tail) return 0;
    *x = input_buffer[tail];
    tail = (tail + 1) & (32 - 1);
    return 1;
}

uint32_t last_time[9];
uint8_t debounce_delay = 5;
uint8_t btn_state[9];
uint8_t last_state[9];
uint8_t curr_state[9];

void load_input_buffer() {
  for (int i = 0; i < 9; i++) {
    curr_state[i] = ~(GPIOC->IDR) >> i & 1;
    if (curr_state[i] != last_state[i]) {
      last_time[i] = HAL_GetTick();
    }
    if ((HAL_GetTick() - last_time[i]) > debounce_delay) {
      if (curr_state[i] != btn_state[i]) {
        btn_state[i] = curr_state[i];
        if (btn_state[i] == 1)
          enqueue(i+1);
      }
    }
    last_state[i] = curr_state[i];
  }
}

uint8_t playing = 0;
volatile int amen_len = sizeof(Amen_Brother_Mono_136_raw) / 2;
uint16_t res;
void read_input_buffer() {
  while(dequeue(&res)) {
    if (res > 0) {
      play_audio((uint16_t *)Amen_Brother_Mono_136_raw, amen_len, &playing);
    }
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  if (hi2s->Instance == SPI2) playing = 0;
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
    TIM2->CCR1 += 84000;
    load_input_buffer();
  }
}
