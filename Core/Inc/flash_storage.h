#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include "learn.h"

uint8_t FlashStorage_SaveModels(const DiodeModel_t *models, uint32_t count);
uint8_t FlashStorage_LoadModels(DiodeModel_t *models, uint32_t count);

#endif
