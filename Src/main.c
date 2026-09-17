#include "main.h"

// Function to configure the GPIO for the LED
void GPIO_Init(void) {
    // Enable the GPIOA peripheral clock
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure PA0 as output
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Output push-pull
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Function to toggle the LED state
void Toggle_LED(void) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
}

int main(void) {
    // Initialize HAL Library
    HAL_Init();

    // Configure the GPIO for the LED
    GPIO_Init();

    // Toggle the LED state every 1 second (1Hz)
    while (1) {
        Toggle_LED();
        HAL_Delay(1000); // Delay for 1 second
    }
}
