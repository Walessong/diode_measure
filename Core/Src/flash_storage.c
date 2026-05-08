#include "flash_storage.h"

#include <string.h>

#define FLASH_PAGE_SIZE_BYTES 1024U
#define FLASH_PAGE_MODEL_ADDR 0x0800FC00UL
#define FLASH_MAGIC_MODEL     0x4D4F4431UL
#define FLASH_VERSION         0x00010003UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    DiodeModel_t data;
    uint32_t checksum;
} ModelRecord_t;

static uint32_t FlashStorage_Checksum(const uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t checksum = 0x13579BDFUL;

    for (i = 0U; i < size; i++)
    {
        checksum = (checksum << 5) | (checksum >> 27);
        checksum ^= data[i];
    }

    return checksum;
}

static uint8_t FlashStorage_WritePage(uint32_t page_address, const void *data, uint32_t size)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    uint32_t i;
    const uint8_t *bytes = (const uint8_t *)data;

    if ((data == NULL) || (size > FLASH_PAGE_SIZE_BYTES))
    {
        return 0U;
    }

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = page_address;
    erase.NbPages = 1U;
    status = HAL_FLASHEx_Erase(&erase, &page_error);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return 0U;
    }

    for (i = 0U; i < size; i += 2U)
    {
        uint16_t halfword = bytes[i];
        if ((i + 1U) < size)
        {
            halfword |= (uint16_t)((uint16_t)bytes[i + 1U] << 8);
        }
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, page_address + i, halfword) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return 0U;
        }
    }

    HAL_FLASH_Lock();
    return 1U;
}

uint8_t FlashStorage_SaveModel(const DiodeModel_t *model)
{
    ModelRecord_t record;

    if (model == NULL)
    {
        return 0U;
    }

    memset(&record, 0xFF, sizeof(record));
    record.magic = FLASH_MAGIC_MODEL;
    record.version = FLASH_VERSION;
    record.data = *model;
    record.checksum = FlashStorage_Checksum((const uint8_t *)&record, sizeof(record) - sizeof(record.checksum));
    return FlashStorage_WritePage(FLASH_PAGE_MODEL_ADDR, &record, sizeof(record));
}

uint8_t FlashStorage_LoadModel(DiodeModel_t *model)
{
    const ModelRecord_t *record = (const ModelRecord_t *)FLASH_PAGE_MODEL_ADDR;
    uint32_t checksum;

    if (model == NULL)
    {
        return 0U;
    }

    checksum = FlashStorage_Checksum((const uint8_t *)record, sizeof(*record) - sizeof(record->checksum));
    if ((record->magic != FLASH_MAGIC_MODEL) ||
        (record->version != FLASH_VERSION) ||
        (record->checksum != checksum) ||
        (record->data.valid == 0U))
    {
        memset(model, 0, sizeof(*model));
        return 0U;
    }

    *model = record->data;
    return 1U;
}
