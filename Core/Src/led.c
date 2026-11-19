#include "led.h"
#include "tim.h"
#include <math.h>
#include <stdint.h>

static uint8_t LED_Data[MAX_LED][4];
static uint8_t LED_Mod[MAX_LED][4];

static volatile int data_sent_flag = 0;

static uint16_t pwm_data[24*MAX_LED+50];
static uint8_t period = 105-1;

void set_LED(int LED_num, int r, int g, int b) {
  if (LED_num < 0 || LED_num >= MAX_LED) return;
  LED_Data[LED_num][0] = LED_num;
  LED_Data[LED_num][1] = g;
  LED_Data[LED_num][2] = r;
  LED_Data[LED_num][3] = b;
}

void set_brightness(int brightness) {
  if (brightness > 45) brightness = 45;

  //scale = 1/tan(90* - b) = tan(b); recomputes when brightness changes
  static int last_brightness = -1;
  static float scale;
  if (brightness != last_brightness) {
    scale = tanf(brightness * (float)M_PI / 180.0f);
    last_brightness = brightness;
  }

  for (int i = 0; i < MAX_LED; i++) {
    LED_Mod[i][0] = LED_Data[i][0];
    for (int j = 1; j < 4; j++)
      LED_Mod[i][j] = (uint8_t)(LED_Data[i][j] * scale);
  }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM1) {
    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
    data_sent_flag = 1;
  }
}

void send_LED() {
  uint32_t indx = 0;
  uint32_t color;

  for (int i = 0; i < MAX_LED; i++) {
    color = LED_Mod[i][1] << 16 | LED_Mod[i][2] << 8 | LED_Mod[i][3];

    for (int i = 23; i >= 0; i--) {

      if (color&(1<<i)) pwm_data[indx] = period * 2/3;
      else pwm_data[indx] = period * 1/3;

      indx++;
    }
  }

  for (int i = 0; i < 50; i++) {
    pwm_data[indx] = 0;
    indx++;
  }

  HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwm_data, indx);
  while (!data_sent_flag){};
  data_sent_flag = 0;

}
int xy(int x, int y) {
  if (y & 1)
    return (WIDTH - 1 - x) + (y * WIDTH);
  return x + (y * WIDTH);
}
