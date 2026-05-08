#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include "learn.h"

uint8_t FlashStorage_SaveModel(const DiodeModel_t *model);
uint8_t FlashStorage_LoadModel(DiodeModel_t *model);

#endif
