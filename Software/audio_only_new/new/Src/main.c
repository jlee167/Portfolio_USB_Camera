/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ov5640.h"
#include "arm_math.h"

#pragma pack(1)
#include "wav_file.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define I2S_SAMPLE_RATE_HZ 		(44100)
#define I2S_SAMPLE_PER_INT 		(I2S_SAMPLE_RATE_HZ / 8)
#define I2S_BUFSIZE_SAMPLES 	(I2S_SAMPLE_PER_INT * 2)		// sampled in stereo, but only single channel used
#define I2S_BUFSIZE_HALFWORD	(I2S_BUFSIZE_SAMPLES * 2)		
#define I2S_BUFSIZE_WORD			(I2S_BUFSIZE_HALFWORD /2)	
#define I2S_QUEUE_LEN  				(6)

#define AMP_FACTOR						(3)


//#define ENABLE_RTC

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2S_HandleTypeDef hi2s1;
DMA_HandleTypeDef hdma_spi1_rx;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd1;

/* USER CODE BEGIN PV */
uint8_t  i2s_queue_head = 0;
uint8_t  i2s_queue_tail = 0;
uint16_t audio_sample_buf[I2S_QUEUE_LEN][I2S_BUFSIZE_HALFWORD];
int16_t audio_fwrite_buf[I2S_SAMPLE_PER_INT];

volatile float32_t iir_in[I2S_SAMPLE_PER_INT];
volatile float32_t iir_out[I2S_SAMPLE_PER_INT];


volatile bool run_state = false;
volatile bool i2s_file_opened = false;

volatile bool dcmi_triggered = false;
volatile bool dcmi_data_valid = false;

volatile FRESULT result_audio_write 	= FR_DENIED;
volatile FRESULT result_audio_close		= FR_DENIED;
volatile FRESULT result_audio_open 		= FR_DENIED;

void indicate_err();
void indicate_success();
void indicate_stop();
void indicate_ready();

void start_recording();
void stop_recording();

void i2s_to_fs();

void file_err_handler(void);

void test_setrtc();
void create_stream_dir();

volatile bool init_done = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S1_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* Number of i2s interrupts */
volatile int i2s_int_cnt = 0;
volatile int i2s_fs_cnt = 0;
volatile int i2s_queue_ovf_cnt = 0;
volatile int fs_fail_cnt = 0;
volatile int exti3_int_cnt = 0;


WAVHeader wav_header;
WAVData wav_data;
char audio_filename[50];
UINT bw;

float32_t iir_coeffs[5] = {
	0.8118496248713318f,
	1.6236992497426637f,
	0.8118496248713318f,
	-1.588572206276975f,
	-0.6588262932083526f
};

const float32_t a0 = 0.8118496248713318f;
const float32_t a1 = 1.6236992497426637f;
const float32_t a2 = 0.8118496248713318f;
const float32_t b1 = 1.588572206276975f;
const float32_t b2 = 0.6588262932083526f;


float32_t in_z1, in_z2, out_z1, out_z2;

float32_t iir_states[4];

arm_biquad_casd_df1_inst_f32 biquad_config;



int16_t Calc_IIR (int16_t inSample) {
	float inSampleF = (float)inSample;
	float outSampleF =
			a0 * inSampleF
			+ a1 * in_z1
			+ a2 * in_z2
			- b1 * out_z1
			- b2 * out_z2;
	in_z2 = in_z1;
	in_z1 = inSampleF;
	out_z2 = out_z1;
	out_z1 = outSampleF;

	return (int16_t) outSampleF;
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

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_I2S1_Init();
  MX_SDMMC1_SD_Init();
  MX_FATFS_Init();
	
	#ifdef ENABLE_RTC
		MX_RTC_Init();
	#endif
  /* USER CODE BEGIN 2 */
	
	
	//HAL_RTC_GetTime(&hrtc, &rtc_time, FORMAT_BIN);
	//HAL_RTC_GetDate(&hrtc, &rtc_date, FORMAT_BIN);
	
	if (HAL_I2S_DeInit(&hi2s1) != HAL_OK)
  {
    Error_Handler();
  }
	
	
	//test_setrtc();
	HAL_Delay(100);
	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	
	
  volatile FRESULT result_mount = FR_INVALID_PARAMETER;
	int mount_fail_cnt = 0;
	
	
	volatile HAL_StatusTypeDef result = HAL_SD_Init(&hsd1);
	while (result != HAL_OK) {
		result = HAL_SD_Init(&hsd1);
	}
	volatile HAL_StatusTypeDef result2 = HAL_SD_InitCard(&hsd1);
	result_mount = f_mount(&SDFatFS, SDPath, 1);
	
	while (result_mount != FR_OK) {
			HAL_SD_DeInit(&hsd1);
			HAL_SD_Init(&hsd1);
			HAL_SD_InitCard(&hsd1);
		
			result_mount = f_mount(&SDFatFS, SDPath, 1);
	};
	
	//create_stream_dir();
	//f_mkdir("image");
	//volatile FRESULT fresult = f_mkdir("audio");
	HAL_Delay(50);
	
	
	for (int cnt=0; cnt < 200; ) {  
		HAL_Delay(10);
		if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3) == GPIO_PIN_RESET){
			cnt = 0;
		} else {
			cnt++;
		}
	}
	
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
	HAL_Delay(50);
	indicate_ready();
	init_done = true;
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if (!run_state) {
			if (i2s_file_opened) {
				i2s_file_opened = false;
				//result_audio_close = FR_DENIED;
				//while (result_audio_close != FR_OK) {
				//	result_audio_close = f_close(&SDFile);
				//}
				result_mount = FR_DENIED;
				
				f_close(&SDFile);
				while (result_mount != FR_OK) {
					HAL_SD_DeInit(&hsd1);
					HAL_SD_Init(&hsd1);
					HAL_SD_InitCard(&hsd1);
					result_mount = f_mount(&SDFatFS, SDPath, 1);
				};
			}
			
			i2s_queue_head = 0;
			i2s_queue_tail = 0;
			//continue;
		} else if (i2s_queue_head != i2s_queue_tail) {
			i2s_to_fs();
		}
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 14;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_SDMMC1
                              |RCC_PERIPHCLK_I2S|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 50;
  PeriphClkInitStruct.PLLI2S.PLLI2SP = RCC_PLLP_DIV2;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 4;
  PeriphClkInitStruct.PLLI2S.PLLI2SQ = 2;
  PeriphClkInitStruct.PLLI2SDivQ = 1;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInitStruct.I2sClockSelection = RCC_I2SCLKSOURCE_PLLI2S;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
}

/**
  * @brief I2S1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S1_Init(void)
{

  /* USER CODE BEGIN I2S1_Init 0 */

  /* USER CODE END I2S1_Init 0 */

  /* USER CODE BEGIN I2S1_Init 1 */

  /* USER CODE END I2S1_Init 1 */
  hi2s1.Instance = SPI1;
  hi2s1.Init.Mode = I2S_MODE_MASTER_RX;
  hi2s1.Init.Standard = I2S_STANDARD_MSB;
  hi2s1.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s1.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s1.Init.AudioFreq = I2S_AUDIOFREQ_44K;
  hi2s1.Init.CPOL = I2S_CPOL_HIGH;
  hi2s1.Init.ClockSource = I2S_CLOCK_PLL;
  if (HAL_I2S_Init(&hi2s1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S1_Init 2 */

  /* USER CODE END I2S1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only 
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
    
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date 
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : IMG_CAPTURE_BTN_Pin */
  GPIO_InitStruct.Pin = IMG_CAPTURE_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(IMG_CAPTURE_BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD0 PD1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PD4 PD5 PD7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{	
	GPIO_TypeDef *port;
	
	switch (GPIO_Pin) {
		case GPIO_PIN_2:
			port = GPIOE;
			break;
		
		case GPIO_PIN_3:
			port = GPIOD;
			break;
	}
	
	
	if (!init_done)
		return;	
	
	HAL_NVIC_DisableIRQ(EXTI2_IRQn);
	HAL_NVIC_DisableIRQ(EXTI3_IRQn);
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
	
	static int debounce_threshhold = 100;
	
	/* Button Debouncing - wait for release */
	for (int cnt=0;;) {  
		HAL_Delay(5);
		
		if (HAL_GPIO_ReadPin(port, GPIO_Pin) == GPIO_PIN_RESET){
			cnt++;
		} else {
			cnt--;
		}
		
		if (abs(cnt) > debounce_threshhold){
			if (cnt < 0) {
				HAL_NVIC_EnableIRQ(EXTI2_IRQn);
				HAL_NVIC_EnableIRQ(EXTI3_IRQn);
				HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
				HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
				__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
				return;
			}
			else
				break;
		}
	}
	
	
	switch(GPIO_Pin) 
	{
		case GPIO_PIN_2:
			break;
			
		case GPIO_PIN_3:
			exti3_int_cnt++;
			indicate_stop();
		
			/* Prepare Directories */
			
		
			if (!run_state){
				create_stream_dir();
				f_mkdir("image");
				f_mkdir("audio");
				start_recording();
			} 
			else {
				stop_recording();
			}
			break;
			
		default:
			break;
	}
	
	/* Button Debouncing - wait for release */
	for (int cnt=0; cnt < debounce_threshhold; ) {
		HAL_Delay(5);
		if ((HAL_GPIO_ReadPin(GPIOD, GPIO_Pin) == GPIO_PIN_SET)) 
			cnt++;
		else
			cnt = 0;
	}
	
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	HAL_NVIC_EnableIRQ(EXTI3_IRQn);
	HAL_NVIC_ClearPendingIRQ(EXTI2_IRQn);
	HAL_NVIC_ClearPendingIRQ(EXTI3_IRQn);
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
}


uint8_t getNextI2sQueueTail() {
	const uint8_t is_last_idx = (i2s_queue_tail == (I2S_QUEUE_LEN-1));
	return is_last_idx ? 0 : i2s_queue_tail+1;
}

uint8_t getNextI2sQueueHead() {
	const uint8_t is_last_idx = (i2s_queue_head == (I2S_QUEUE_LEN-1));
	return is_last_idx ? 0 : i2s_queue_head+1;
}


void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	i2s_int_cnt++;
	
	const bool i2s_queue_full = (i2s_queue_head == getNextI2sQueueTail());
	const uint8_t is_last_idx = (i2s_queue_tail == (I2S_QUEUE_LEN-1));
	
	if (i2s_queue_full) {
		HAL_I2S_Receive_DMA(&hi2s1, audio_sample_buf[i2s_queue_tail], I2S_BUFSIZE_WORD);
		i2s_queue_ovf_cnt++;
		return;
	}
	
	i2s_queue_tail = getNextI2sQueueTail();
	
	/* Issue I2S Read */
	while (HAL_I2S_GetState(&hi2s1) != HAL_I2S_STATE_READY) {};
	HAL_I2S_Receive_DMA(&hi2s1, audio_sample_buf[i2s_queue_tail], I2S_BUFSIZE_WORD);
}




void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s) {
}


void indicate_err() {
	for (int i = 0; i < 10; i++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_Delay(100);
	}
}

void indicate_fault() {
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
}


/********************************
		LED Control Functions
*********************************/

void indicate_success() {
	for (int i = 0; i < 2; i++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_Delay(500);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_Delay(500);
	}
}

void indicate_ready() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
}


void indicate_stop() {
	for (int i = 0; i < 4; i++){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_Delay(200);
	}
}


void stop_recording() {
	
	run_state = false;

	/* Stop I2S */
	//if (HAL_I2S_DeInit(&hi2s1) != HAL_OK)
	//{
	//	Error_Handler();
	//}
	HAL_I2S_DMAStop(&hi2s1);
	HAL_NVIC_DisableIRQ(SPI1_IRQn);
	HAL_NVIC_ClearPendingIRQ(SPI1_IRQn);
	HAL_Delay(500);
	
	/* Turn off Indicator LED */
	indicate_stop();

}


void start_recording() {
	
	indicate_success();
	
	/* Open FatFS filestreams */
	char img_idx_str[20]="";
	sprintf(img_idx_str, "%d", 2);

	
	strcpy(audio_filename, "");
	f_getcwd(audio_filename, 20);
	strcat(audio_filename, "/audio/audio");
	//strcat(audio_filename, img_idx_str);
	strcat(audio_filename, ".wav");		
	
	init_default(&wav_header);
	
	
	result_audio_open = create_wav_fatfs(&SDFile, audio_filename, &wav_header);  
	
	if (result_audio_open != FR_OK) {
		indicate_err();
		return;
	}
	i2s_file_opened = true;
	
	/* Initialize Audio Filter */
	arm_biquad_cascade_df1_init_f32(&biquad_config, 1, iir_coeffs, iir_states);
	
	/* Start I2S audio */
	if (HAL_I2S_Init(&hi2s1) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_NVIC_EnableIRQ(SPI1_IRQn);
	HAL_Delay(500);
	
	HAL_I2S_Receive_DMA(&hi2s1, audio_sample_buf[i2s_queue_tail], I2S_BUFSIZE_WORD);
	
	/* Turn on Indicator LED */
	run_state = true;
	indicate_ready();
}


void i2s_to_fs() {
	volatile int start_idx = 0;
	volatile int len = 0;
	UINT bw;
	volatile uint16_t val;
	
	if (i2s_queue_head == i2s_queue_tail)
		return;
	
	
	/* Search for first half-word data of i2s audio buffer */
	for (start_idx = 0; start_idx < 4; start_idx++) {
		if ((audio_sample_buf[i2s_queue_head][start_idx] & 0x1fff) != 0x0000)
			break;
	}
	
	/* Current I2S setting: 18 bits sample from 32 bit data frame. 
	   Extract most significant 16 bit only. */
	for (len = 0; (len*4 + start_idx) < I2S_BUFSIZE_HALFWORD; len++) { 
		int idx_buf = len;
		int idx_sample = len*4 + start_idx;
		int16_t wval =   ((audio_sample_buf[i2s_queue_head][idx_sample] << 1) 
											| (audio_sample_buf[i2s_queue_head][idx_sample+1] >> 15));
		
		//audio_fwrite_buf[len] = Calc_IIR(wval)*3;//wval;
		iir_in[len] = (float32_t) wval * AMP_FACTOR;
	}	
	
	arm_biquad_cascade_df1_f32(&biquad_config, iir_in, iir_out, len);
	
	
	for (int i = 0; i < len; i++) {
		audio_fwrite_buf[i] = (int16_t) iir_out[i];
	}
	result_audio_write = write_pcm_data(&SDFile, audio_fwrite_buf, len);
	
	if (result_audio_write != FR_OK) {
		static int reopen_cnt = 0;
		reopen_cnt++;
		
		FRESULT result_reopen = FR_DENIED;
		while (result_reopen != FR_OK) {
			f_close(&SDFile);
			result_reopen = f_open(&SDFile, audio_filename, FA_WRITE | FA_OPEN_APPEND | FA_READ);
		}
	}
	
	if ((!(i2s_fs_cnt%100)) && (i2s_fs_cnt > 0)) {
		f_sync(&SDFile);
		//f_close(&SDFile);
		//result_audio_open = f_open(&SDFile, audio_filename, FA_WRITE | FA_OPEN_APPEND | FA_READ);
	}
	
	i2s_queue_head = getNextI2sQueueHead();
	
	i2s_fs_cnt++;
}




void file_err_handler(void) {
	
	fs_fail_cnt++;
	indicate_err();
	
	FRESULT audio_closed = f_close(&SDFile);
	
	if (audio_closed != FR_OK) {
		HAL_SD_InitCard(&hsd1);
		f_mount(&SDFatFS, SDPath, 1);
	}
	
	FRESULT audio_opened = f_open(&SDFile, audio_filename, FA_WRITE | FA_OPEN_APPEND);
	
	if (audio_opened != FR_OK)
		file_err_handler();
}


#ifdef ENABLE_RTC
void test_setrtc() {
	RTC_TimeTypeDef rtc_time;
	RTC_DateTypeDef rtc_date;
	RTC_AlarmTypeDef sAlarm;
	
	rtc_date.Year = 22;
	rtc_date.Month = 9;
	rtc_date.Date = 18;
	
	rtc_time.Hours = 13;
	rtc_time.Minutes = 04;
	rtc_time.Seconds = 0;
	
	
	HAL_RTC_SetTime(&hrtc, &rtc_time, FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &rtc_date, FORMAT_BIN);
}
#endif


void create_stream_dir() {
	
	#ifdef ENABLE_RTC
		RTC_TimeTypeDef rtc_time;
		RTC_DateTypeDef rtc_date;
		RTC_AlarmTypeDef sAlarm;
		char date_str[80];
		
		HAL_RTC_GetTime(&hrtc, &rtc_time, FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &rtc_date, FORMAT_BIN);
		
		sprintf(date_str, "%02d%02d%02d%02d%02d%02d", rtc_date.Year, rtc_date.Month , rtc_date.Date,
																						rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
		
		volatile FRESULT result; 
		result = f_chdir("/");
		result = f_mkdir(date_str);
		result = f_chdir(date_str);
	
		
	#else
		volatile FRESULT result; 
		
		DIR DIR;
		for (int i=0;;i++) {
			char dirname[6];
			sprintf(dirname, "%d", i);
			
			f_chdir("/");
			result = f_opendir(&DIR, dirname);
			f_closedir(&DIR);
			if (result == FR_OK){
				continue;
			}
			else if (result == FR_NO_PATH) {
				result = f_chdir("/");
				result = f_mkdir(dirname);
				result = f_chdir(dirname);
				break;
			} 
			else {
				indicate_err();
			}
		}
		
		
	#endif
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

	while (true) {
		indicate_stop();
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
