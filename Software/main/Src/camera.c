#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_dcmi.h"
#include "ov5640.h"
#include "fatfs.h"
#include "sdio.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef hdma_dcmi;

volatile FRESULT result_video_write = FR_DENIED;
volatile FRESULT result_video_close = FR_DENIED;
volatile FRESULT result_video_open = FR_DENIED;

volatile bool dcmi_data_valid = false;
volatile int dcmi_fs_cnt = 0;

void init_camera(void)
{
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);
  while (OV5640_Init())
  {
    HAL_Delay(300);
  }

  OV5640_JPEG_Mode();
  OV5640_Focus_Init();
  OV5640_Light_Mode(0);       // set auto
  OV5640_Color_Saturation(3); // default
  OV5640_Brightness(4);       // default
  OV5640_Contrast(3);         // default
  OV5640_Sharpness(33);       // set auto
  OV5640_Auto_Focus();

  HAL_Delay(50);
}

void stop_camera(void)
{
  HAL_DCMI_Stop(&hdcmi);
}

void start_camera()
{
  jpeg_test(QVGA_320_240);
}

void dcmi_to_fs(DCMI_HandleTypeDef *hdcmi)
{
  volatile uint8_t *p;
  volatile uint32_t i = 0, jpgstart = 0, jpglen = 0;
  volatile uint8_t head = 0;
  extern uint32_t jpeg_data_buf[30 * 1024];

  p = (uint8_t *)jpeg_data_buf;

  for (i = 0; i < jpeg_buf_size * 4; i++) // search for 0XFF 0XD8 and 0XFF 0XD9, get size of JPG
  {
    if ((p[i] == 0XFF) && (p[i + 1] == 0XD8))
    {
      jpgstart = i;
      head = 1; //
    }
    if ((p[i] == 0XFF) && (p[i + 1] == 0XD9) && head)
    {
      jpglen = i - jpgstart + 2;
      break;
    }
  }
  if (jpglen)
  {
    p += jpgstart; // move to FF D8

    static int img_idx = 0;
    char img_fname[30] = "";
    char img_idx_str[20] = "";
    sprintf(img_idx_str, "%d", img_idx);
    UINT bw;

    /* SD Test Codes */
    strcat(img_fname, "/image/");
    strcat(img_fname, img_idx_str);
    strcat(img_fname, ".jpeg");
    result_video_open = f_open(&VideoFile, img_fname, FA_WRITE | FA_CREATE_ALWAYS);

    if (result_video_open != FR_OK)
    {
      HAL_DCMI_Start_DMA(hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)jpeg_data_buf, jpeg_buf_size / 4);
      return;
    }

    result_video_write = f_write(&VideoFile, (void *)p, jpglen, &bw);
    result_video_close = FR_DENIED;

    if (result_video_write != FR_OK)
    {
      f_close(&VideoFile);
      HAL_DCMI_Start_DMA(hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)jpeg_data_buf, jpeg_buf_size / 4);
      return;
    }

    for (int i = 0; i < 100; i++)
    {
      result_video_close = f_close(&VideoFile);
      if (result_video_close == FR_OK)
      {
        img_idx++;
        return;
      }
    }

    sd_err_handler();
  }

  HAL_DCMI_Start_DMA(hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)jpeg_data_buf, jpeg_buf_size / 4);
  __HAL_DCMI_ENABLE_IT(hdcmi, DCMI_IT_FRAME);
}

void app_background_camera(DCMI_HandleTypeDef *hdcmi)
{
  if (dcmi_data_valid)
  {
    dcmi_to_fs(hdcmi);
    dcmi_data_valid = false;
    dcmi_fs_cnt++;
  }
}