/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <arm_math.h>
#include "semphr.h"
#include "32x32.h"
#include "user_button.h"
#include <stdlib.h>
#include "uart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define TEST_PATTERN

#define DEVICE_STATE_OFF 0
#define DEVICE_STATE_ON 1

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
//UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim3;

//osThreadId defaultTaskHandle;

TaskHandle_t WriteLEDTaskHandler, vSyncTaskHandler;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM3_Init(void);
void Onboard_LED_Init(void);

void WriteLEDTask(void *argument);
void vSyncTask(void *argument);

void power_off(void);
void power_on(void);
void write_matrix(uint8_t rgb_data[32*32*3]);

void vApplicationIdleHook(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// make the startup script happy
//void _init(void) {}
// make the dsp libs happy
int __errno;

// each uint32_t has 8 4bit nibbles representing
uint32_t *image_buffer;
uint32_t backbuffer[2][4*3*32];
volatile uint8_t backbuffer_index = 0;
static xSemaphoreHandle xVsyncSemaphore = NULL;
static xSemaphoreHandle xFrameReadySemaphore = NULL;


uint8_t rx_image_data[32*32*3];
uint8_t saved_image_data[32*32*3];
uint8_t curr_image_data[32*32*3];
Color LED_Data[MATRIX_HEIGHT][MATRIX_WIDTH];

const uint32_t bit_angle_modulation_lookup[SCAN_RATE] = {
		0x1u,
		0x2u,
		0x2u,
		0x4u,
		0x4u,
		0x4u,
		0x4u,
		0x8u,
		0x8u,
		0x8u,
		0x8u,
		0x8u,
		0x8u,
		0x8u,
		0x8u
};


/* USER CODE END 0 */

/**
  * @brief  The application entry point. Starts by writing a default pattern to the LED matrix
  * @retval int
  * Source: https://github.com/cpmetz/32x32-instructable/blob/master/main.patch
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	BaseType_t retVal;	// used for checking task creation

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
  MX_TIM3_Init();
  UserButton_Init();
  UART_Init();

  UART_Reset_Terminal();


//  Onboard_LED_Init();  /* USER CODE BEGIN 2 */

      image_buffer = backbuffer[backbuffer_index];
      int curr_i = 0;
  #ifdef TEST_PATTERN
    for(int y=0;y<32;++y) {
  	  for(int x=0;x<32;++x)
  	  {
  		  // layout: b - g - r
  		  image_buffer = backbuffer[0];
  		  set_pixel(x,y, x/2, y/2, (63-(x+y))/4);

  		  image_buffer = backbuffer[1];
  		  set_pixel(x,y, x/2, y/2, (63-(x+y))/4);

  		  curr_image_data[curr_i] = x/2;
  		  curr_image_data[curr_i+1] = y/2;
  		  curr_image_data[curr_i+2] = (63-(x+y))/4;

  		  curr_i += 3;

  	  }
    }
  #endif

    xVsyncSemaphore = xSemaphoreCreateBinary();
    xFrameReadySemaphore = xSemaphoreCreateBinary();

    if (xFrameReadySemaphore == NULL) {
        while(1);
    }

    if (xVsyncSemaphore == NULL) {
		while(1);
    }


  // Create tasks
    retVal = xTaskCreate(WriteLEDTask, "WriteLEDTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &WriteLEDTaskHandler);
    if (retVal != pdPASS) { while(1);}	// check if task creation failed

    retVal = xTaskCreate(vSyncTask, "vSyncTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &vSyncTaskHandler);
    if (retVal != pdPASS) { while(1);}	// check if task creation failed

  /* Start scheduler */
//  osKernelStart();
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 32;
//  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLP = 0x4U;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
//  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
//  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 31;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 129; // 129
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, D_A_Pin|D_B_Pin|D_C_Pin|D_D_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, D_OE_Pin|D_STB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : D_A_Pin D_B_Pin D_C_Pin D_D_Pin */
  GPIO_InitStruct.Pin = D_A_Pin|D_B_Pin|D_C_Pin|D_D_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : D_OE_Pin D_STB_Pin */
  GPIO_InitStruct.Pin = D_OE_Pin|D_STB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

static uint16_t line = 0;
static uint8_t cur_linedata = 0;
static uint32_t linedata[2][6];
static volatile uint8_t scan = 0;
static volatile uint32_t frame = 0;

static volatile uint8_t transfer_pending = 0;
static volatile uint16_t spi_transmit_line = 0;

/**
  * @brief Uses GPIO to toggle matrix lines to be powered on, then writes the LED data to the lines
  * @retval None
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi == &hspi1)
	{
		// drive OE high
		GPIOB->BSRR = GPIO_PIN_0;

		GPIOC->ODR = spi_transmit_line & 0xF;
    
		// latch data in
		GPIOB->BSRR = GPIO_PIN_1;

		// give the IOs some time to settle
		__asm volatile (
				" nop	\n"
				" nop	\n"
				" nop	\n"
				" nop	\n"
				);

		// disable latches
		GPIOB->BSRR = GPIO_PIN_1 << 16;

		// drive OE low
		GPIOB->BSRR = GPIO_PIN_0 << 16;

		transfer_pending = 0;

//		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/* Source: https://github.com/cpmetz/32x32-instructable/blob/master/main.patch */
uint32_t bitline(const uint32_t *image_data, const uint8_t bam_pattern)
{
	uint32_t v = 0;
	const uint32_t bam_pattern32 = (uint32_t)bam_pattern;
	for(uint8_t i=0;i<4;++i)
	{
		const uint32_t id = *image_data++;
		uint32_t bam_shifter = bam_pattern32;
		for(uint8_t j=0;j<8;++j)
		{
			v <<= 1;
			if((id & bam_shifter)>0)
				v |= 0x1u;
			bam_shifter <<= 4;
		}
	}

	return v;
}

uint8_t isr_continue_flag = 0;
uint8_t task_continue_flag = 0;

/**
  * @brief Given input RGB data, writes the data to the LED matrix
  * @param: rgb_data: Input array of RGB data of size (HEIGHT * WIDTH * 3)
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim == &htim3)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR(WriteLEDTaskHandler, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}



/**
  * @brief Task for transfering RGB data to matrix via SPI/DMA
  * @param: argument: not used
  * @retval None
  * Source: https://github.com/cpmetz/32x32-instructable/blob/master/main.patch
  */
void WriteLEDTask(void * argument) {
	for(;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);		// wait for notification from HAL_TIM_PeriodElapsedCallback
		if ( !transfer_pending )
				{
					transfer_pending = 1;

					// dispatch SPI transfer in the background
					spi_transmit_line = line;
					HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)&linedata[cur_linedata][0], sizeof(uint32_t)*6);

					++line;

					// we only have 15 slots
					if(line>0xF) {
						line = 0;
						++scan;
					}
					//  times oversampling
					if(scan>SCAN_RATE) {
						scan = 0;
						++frame;

						if(xFrameReadySemaphore != NULL)
						{
							if (xSemaphoreTake(xFrameReadySemaphore, 0) == pdTRUE)
							{
								backbuffer_index ^= 1;
							}
						}

						// trigger vSync
						if(xVsyncSemaphore != NULL)
						{
							signed portBASE_TYPE highPrioWoven;
							xSemaphoreGiveFromISR(xVsyncSemaphore, &highPrioWoven);
						}
					}

					// switch to other buffer to populate SPI data
					cur_linedata ^= 1;
					/* prepare next line */
					uint32_t *ld = &(linedata[cur_linedata][0]);

					const uint32_t *display_buffer = &backbuffer[backbuffer_index^1][0];
					const uint32_t *lp_top = display_buffer + line*4*3;
					const uint32_t *lp_bottom = display_buffer + line*4*3 + 16*4*3;
					const uint8_t bam_pattern = bit_angle_modulation_lookup[scan];

					*ld++ = __REV(bitline(lp_bottom, bam_pattern));
					*ld++ = __REV(bitline(lp_bottom+4, bam_pattern));
					*ld++ = __REV(bitline(lp_bottom+8, bam_pattern));
					*ld++ = __REV(bitline(lp_top, bam_pattern));
					*ld++ = __REV(bitline(lp_top+4, bam_pattern));
					*ld   = __REV(bitline(lp_top+8, bam_pattern));
				}
	}
}

/**
  * @brief Task for triggering each V-Sync on the LED display
  * @param: argument: not used
  * @retval None
  * Source: https://github.com/cpmetz/32x32-instructable/blob/master/main.patch
  */
void vSyncTask(void * argument)
{
  /* USER CODE BEGIN 5 */

		float t = 0;
		GPIOB->BSRR = GPIO_PIN_0 << 16;

		HAL_TIM_Base_Start_IT(&htim3);

		/* Infinite loop */
		for(;;)
		{
			// wait forever for a vSync
			if ( xVsyncSemaphore != NULL)
			{
				xSemaphoreTake(xVsyncSemaphore, portMAX_DELAY);
			}

			xSemaphoreGive(xFrameReadySemaphore);
		}
  /* USER CODE END 5 */
}

uint8_t device_state = DEVICE_STATE_ON;

/**
  * @brief Saves LED image to an array, disables UART IRQ, then clears the LED matrix upon power off.
  * @retval None
  */
void power_off(void) {
	memcpy(saved_image_data, curr_image_data, sizeof(uint8_t)*32*32*3);
	HAL_NVIC_DisableIRQ(USART2_IRQn);
	clear_led_matrix();
//	HAL_NVIC_DisableIRQ(DMA2_Channel4_IRQn);

}

/**
  * @brief Re-enables the UART IRQ and writes the saved image back to the matrix
  * @retval None
  */
void power_on(void) {
	HAL_NVIC_EnableIRQ(USART2_IRQn);
	write_matrix(saved_image_data);
//	HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);
}


/**
  * @brief User Button IRQ Handler, turns device either on or off
  * @retval None
  */
void EXTI15_10_IRQHandler(void) {
	if (EXTI->PR1 & EXTI_PR1_PIF13) {

		device_state ^= 1;

		if (device_state == DEVICE_STATE_ON) {
			power_on();
		} else if (device_state == DEVICE_STATE_OFF) {
			power_off();
		}

		EXTI->PR1 = EXTI_PR1_PIF13;	// clear flag
	}
}

int rx_index = 0;

/**
  * @brief UART interrupt handler. Stores the RGB data sent from the python script, then writes the data to the matrix
  * @retval None
  */
void USART2_IRQHandler(void) {
	// If read data register not empty, store the data (keypress) in global variable and set key press flag high

	if ((USART2->ISR & USART_ISR_RXNE) != 0) {
		int data = USART2->RDR;

		rx_image_data[rx_index++] = data >> 6;

		if (rx_index >= (32*32*3)) {
			rx_index = 0;

			// Write image data to the led_matrix array
			write_matrix(rx_image_data);

			memset(rx_image_data, 0, sizeof(rx_image_data));

		}

	}

}

/**
  * @brief Triggers tickless idle low power mode once the application is idle
  * @retval None
  */
void vApplicationIdleHook(void) {
	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}

/**
  * @brief Given input RGB data, writes the data to the LED matrix
  * @param: rgb_data: Input array of RGB data of size (HEIGHT * WIDTH * 3)
  * @retval None
  */
void write_matrix(uint8_t rgb_data[32*32*3]) {
	memcpy(curr_image_data, rgb_data, sizeof(uint8_t)*32*32*3);
	for (int i = 0; i < 32 * 32 * 3; i += 3) {
		int x = (i / 3) % 32;
		int y = (i / 3) / 32;
		int r = rgb_data[i];
		int g = rgb_data[i+1];
		int b = rgb_data[i+2];
		image_buffer = backbuffer[0];
		  set_pixel(x,y, r, g, b);

		  image_buffer = backbuffer[1];
		  set_pixel(x,y, r, g, b);
	}
}

/**
  * @brief Init function for onboard LED for debugging
  * @retval None
  */
void Onboard_LED_Init(void) {

	// Configure onboard LED
	  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

	  GPIOA->MODER 	&= 	~(GPIO_MODER_MODE5); 			// Clear MODE5 (bits [11:10])
	  GPIOA->MODER 	|= 	(1 << GPIO_MODER_MODE5_Pos); 	// set MODE5 = 01

	  GPIOA->OTYPER &= 	~(GPIO_OTYPER_OT5);			// push-pull off
	  GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED5);	// low speed
	  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5);			// no pull up/down
}

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

#ifdef  USE_FULL_ASSERT
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
