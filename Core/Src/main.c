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
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void print_r1(uint8_t r1) {
  printf("%02X\n", r1);
  if (r1 >> 0 & 1) printf("In Idle State\n");
  if (r1 >> 1 & 1) printf("Erase State\n");
  if (r1 >> 2 & 1) printf("Illegal Command\n");
  if (r1 >> 3 & 1) printf("Com CRC Error\n");
  if (r1 >> 4 & 1) printf("Erase Sequence Error\n");
  if (r1 >> 5 & 1) printf("Address Error\n");
  if (r1 >> 6 & 1) printf("Parameter Error\n");
}

void print_r7(uint32_t r7) {
  printf("%08lX\n", (unsigned long)r7);

  if (r7 >> 13 & 1) printf("PCIe 1.2V Support Accepted\n");

  uint8_t volt = (r7 >> 8) & 0x0F;
  if (volt == 0x0)      printf("Voltage Not Defined\n");
  else if (volt == 0x1) printf("2.7-3.6V\n");
  else if (volt == 0x2) printf("Reserved for Low Voltage Range\n");
  else                  printf("Unknown voltage: %X\n", volt);
}

void print_r3(uint32_t r3) {
  printf("%08lX\n", (unsigned long)r3);
}
/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  const uint8_t CMD0[]   = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
  const uint8_t CMD8[]   = {0x48, 0x00, 0x00, 0x01, 0xAA, 0x87};
  const uint8_t CMD55[]  = {0x77, 0x00, 0x00, 0x00, 0x00, 0x01};
  const uint8_t ACMD41[] = {0x69, 0x40, 0x00, 0x00, 0x00, 0x01};
  const uint8_t CMD58[]  = {0x7A, 0x00, 0x00, 0x00, 0x00, 0x01};
  uint8_t ff = 0xFF;

  // power up send 80 pulses of FF with CS high
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  for (int i = 0; i < 10; i++) HAL_SPI_Transmit(&hspi1, &ff, 1, 100);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  // send CMD0
  HAL_SPI_Transmit(&hspi1, (uint8_t *)CMD0, 6, 100);

  //response will come within 0-8 cycles
  uint8_t r1 = 0xFF;
  for (int i = 0; i < 8 && r1 == 0xFF; i++)
    HAL_SPI_TransmitReceive(&hspi1, &ff, &r1, 1, 100);
  printf("CMD0 Response:\n");
  print_r1(r1);
  printf("\n");

  HAL_SPI_Transmit(&hspi1, (uint8_t *)CMD8, 6, 100);

  r1 = 0xFF;
  for (int i = 0; i < 8 && r1 == 0xFF; i++)
    HAL_SPI_TransmitReceive(&hspi1, &ff, &r1, 1, 100);
  printf("CMD8 Response:\n");
  print_r1(r1);

  uint32_t r7 = 0;
  for (int i = 0; i < 4; i++) {
    uint8_t r;
    HAL_SPI_TransmitReceive(&hspi1, &ff, &r, 1, 100);
    r7 = (r7 << 8) | r;
  }
  print_r7(r7);
  printf("\n");

  printf("Initializing...\n");
  do {
    HAL_SPI_Transmit(&hspi1, (uint8_t *)CMD55, 6, 100);
    r1 = 0xFF;
    for (int i = 0; i < 8 && r1 == 0xFF; i++)
      HAL_SPI_TransmitReceive(&hspi1, &ff, &r1, 1, 100);

    HAL_SPI_Transmit(&hspi1, (uint8_t *)ACMD41, 6, 100);
    r1 = 0xFF;
    for (int i = 0; i < 8 && r1 == 0xFF; i++)
      HAL_SPI_TransmitReceive(&hspi1, &ff, &r1, 1, 100);

  } while (r1 == 0x01);

  HAL_SPI_Transmit(&hspi1, (uint8_t *)CMD58, 6, 100);
  r1 = 0xFF;
  for (int i = 0; i < 8 && r1 == 0xFF; i++)
    HAL_SPI_TransmitReceive(&hspi1, &ff, &r1, 1, 100);

  uint32_t r3 = 0;
  for (int i = 0; i < 4; i++) {
    uint8_t r;
    HAL_SPI_TransmitReceive(&hspi1, &ff, &r, 1, 100);
    r3 = (r3 << 8) | r;
  }
  if (r3 >> 30 & 1) printf("SDHC/SDXC Ready...\n");
  else printf("SDSC Ready...\n");



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
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
