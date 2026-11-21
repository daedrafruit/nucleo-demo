#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "tim.h"
#include "input.h"

volatile input_event_t input_buffer[32];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

// SOURCE: https://quant.engineering/ring-buffers-101.html
uint8_t enqueue(input_event_t press) {
  int next = (head + 1) & (32 - 1);
  if (next == tail) return 0;
  input_buffer[head] = press;
  head = next;
  return 1;
}

uint8_t dequeue(input_event_t *press) {
  if (head == tail) return 0;
  *press = input_buffer[tail];
  tail = (tail + 1) & (32 - 1);
  return 1;
}

uint32_t last_time[9];
uint8_t debounce_delay = 5;
uint8_t btn_state[9];
uint8_t last_state[9];
uint8_t curr_state[9];

//TODO: play audio immediately for low latency feel, queue input with timestamp for later timing processing
void load_input_buffer(uint32_t chart_time) {
  uint32_t now = HAL_GetTick();
  for (int i = 0; i < 9; i++) {
    uint8_t curr = ~(GPIOC->IDR) >> i & 1;
    if (curr != btn_state[i] && now - last_time[i] > debounce_delay) {
      btn_state[i] = curr;
      last_time[i] = now;
      if (curr == 1) {
        //TODO: lanes start count at 1 (0 means no event), this system needs to be improved
        enqueue((input_event_t){chart_time, i + 1});
      }
    }
  }
}

input_event_t res;
uint8_t poll_input_event(input_event_t *e) {
  if (dequeue(e))
    return 1;
  else
    return 0;
}

void init_input_buffer() {
  HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
}
