#include "flash_storage.h"

#include <string.h>

#define FLASH_PAGE_SIZE_BYTES 1024U
#define FLASH_PAGE_MODEL_ADDR 0x0800FC00UL
#define FLASH_MAGIC_MODEL     0x4D4F4432UL
#define FLASH_VERSION         0x00010006UL

typedef struct
{
    uint32_t magic;
    uint32_t version;
    DiodeModel_t models[MODEL_SLOT_COUNT];
    uint32_t checksum;
} ModelStoreRecord_t;

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

uint8_t FlashStorage_SaveModels(const DiodeModel_t *models, uint32_t count)
{
    ModelStoreRecord_t record;
    uint32_t copy_count;

    if ((models == NULL) || (count == 0U))
    {
        return 0U;
    }

    memset(&record, 0xFF, sizeof(record));
    record.magic = FLASH_MAGIC_MODEL;
    record.version = FLASH_VERSION;
    copy_count = (count > MODEL_SLOT_COUNT) ? MODEL_SLOT_COUNT : count;
    memcpy(record.models, models, copy_count * sizeof(DiodeModel_t));
    record.checksum = FlashStorage_Checksum((const uint8_t *)&record, sizeof(record) - sizeof(record.checksum));
    return FlashStorage_WritePage(FLASH_PAGE_MODEL_ADDR, &record, sizeof(record));
}

uint8_t FlashStorage_LoadModels(DiodeModel_t *models, uint32_t count)
{
    const ModelStoreRecord_t *record = (const ModelStoreRecord_t *)FLASH_PAGE_MODEL_ADDR;
    uint32_t checksum;
    uint32_t copy_count;

    if ((models == NULL) || (count == 0U))
    {
        return 0U;
    }

    checksum = FlashStorage_Checksum((const uint8_t *)record, sizeof(*record) - sizeof(record->checksum));
    if ((record->magic != FLASH_MAGIC_MODEL) ||
        (record->version != FLASH_VERSION) ||
        (record->checksum != checksum))
    {
        memset(models, 0, count * sizeof(DiodeModel_t));
        return 0U;
    }

    copy_count = (count > MODEL_SLOT_COUNT) ? MODEL_SLOT_COUNT : count;
    memcpy(models, record->models, copy_count * sizeof(DiodeModel_t));
    if (count > copy_count)
    {
        memset(&models[copy_count], 0, (count - copy_count) * sizeof(DiodeModel_t));
    }
    return 1U;
}
