#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "dac.h"
#include "tim.h"

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
  uint32_t now = HAL_GetTick();
  for (int i = 0; i < 9; i++) {
    uint8_t curr = ~(GPIOC->IDR) >> i & 1;
    if (curr != btn_state[i] && now - last_time[i] > debounce_delay) {
      btn_state[i] = curr;
      last_time[i] = now;
      if (curr == 1)
        enqueue(i + 1);
    }
  }
}

uint16_t res;
void read_input_buffer() {
  while(dequeue(&res)) {
    if (res > 0) {
      play_audio(res - 1);
    }
  }
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
    TIM2->CCR1 = TIM2->CNT + 84000;
    load_input_buffer();
  }
}

void init_input_buffer() {
  HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
}
