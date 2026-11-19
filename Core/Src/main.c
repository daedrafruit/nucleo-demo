/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2s.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#include "sd.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/_intsup.h>

#include "led.h"
#include "input.h"
#include "dac.h"
#include "sd.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef struct {
  uint32_t time;
  uint8_t lanes;
} note_t;

note_t rows[] = {
  {300, 0b11111111},
  {375, 0b01000000},
  {450, 0b00010000},
  {600, 0b00000100},
  {825, 0b00000001},
  {975, 0b00000100},
  {1350, 0b00010000},
  {1650, 0b00100000},
  {1875, 0b00001000},
  {2100, 0b10001001},
  {2475, 0b00100110},
  {2700, 0b01000000},
  {3150, 0b00010001},
  {3450, 0b00000100},
  {4050, 0b10000000},
  {4200, 0b00100000},
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define FRAME_SIZE 19
#define SCROLL_SPEED 75
uint32_t time_frame = FRAME_SIZE * SCROLL_SPEED;
uint8_t base = 0;
#define FRAME_MS 33
uint32_t num_rows = sizeof(rows) / sizeof(rows[0]);
uint32_t chart_time;
uint32_t song_start;

// ms since song start, looping at the last note
uint32_t get_chart_time() {
  return (HAL_GetTick() - song_start) % rows[num_rows - 1].time;
}

//TODO: bug, final row is not properly cleared
void update_display() {
  //skip first row (red bar)
  for (int i = 1; i < FRAME_SIZE; i++)
    for (int j = 0; j <= 7; j++)
      set_LED(xy(j, i), 0, 0, 0);

  for (int i = 0; i < num_rows; i++) {
    if (rows[i].time < chart_time) continue;

    uint8_t y_pos = (rows[i].time - chart_time) / SCROLL_SPEED;
    if (y_pos >= FRAME_SIZE || y_pos < 1) continue;

    //first col always red
    if (rows[i].lanes >> 7 & 1)
      set_LED(xy(0, y_pos), 255, 0, 0);

    for (int j = 0; j < 7; j++)
      if (rows[i].lanes >> j & 1)
        //reverse for endianness
        if (j & 1)
          set_LED(xy(7-j, y_pos), 0, 0, 255);
        else
          set_LED(xy(7-j, y_pos), 255, 255, 255);
  }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_I2S2_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  if (sd_init() != 0) 
    printf("ERROR: SD init failed\n");
  init_input_buffer();
  audio_set_rate(16000);
  audio_init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  for (int i = 0; i < 8; i++) {
    set_LED(xy(i, 0), 255, 0, 0);
  }
  song_start = HAL_GetTick();
  uint32_t last_frame = song_start;
  while (1) {
    if (HAL_GetTick() - last_frame >= FRAME_MS) {
      last_frame += FRAME_MS;
      chart_time = get_chart_time();
      update_display();
      set_brightness(1);
      send_LED();
    }

    read_input_buffer();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
    // 1ms at 84MHz
    TIM2->CCR1 = TIM2->CNT + 84000;


    load_input_buffer(get_chart_time());
  }
}

int _write(int fd, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
