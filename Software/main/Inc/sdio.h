#ifndef __SDIO_H__
#define __SDIO_H__

#include "fatfs.h"

void sd_err_handler();
void init_sd_fatfs(void);

#endif