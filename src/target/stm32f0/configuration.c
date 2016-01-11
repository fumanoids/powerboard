/*
 * configuration.c
 *
 *  Created on: 05.05.2012
 *      Author: danielb
 */

#include <libopencm3/stm32/rcc.h>
#include <flawless/init/systemInitializer.h>

#include <interfaces/configuration.h>
#include <interfaces/led.h>

#include <libopencm3/stm32/flash.h>
#include <string.h>

#define FLASH_CONFIG_PAGE 63

extern uint16_t _flashConfigROMBegin;
extern uint16_t _flashConfigROMEnd;

extern const configVariableDescriptor_t _applicationConfigDescriptorsBegin, _applicationConfigDescriptorsEnd;

#define FLASH_CONFIG_SECTOR (11 << 3)
#define FLASH_PROGRAMM_ACCESS_SIZE (1 << 8)

#define FLASH_CONFIG_SECTOR_SIZE (128 * 1024)
#define FLASH_CONFIG_START_ADDR   (0x0800FC00)

#define IS_POINTER_IN_CONFIG_MEMORY(ptr) (((void*)(ptr) >= (void*)&_flashConfigROMBegin) && ((void*)(ptr) < (void*)&_flashConfigROMEnd))

typedef struct tag_configRecordEntry
{
	const configVariableDescriptor_t* configDescriptorPtr;
	const void *configDataPtr;
} configRecordEntry_t;


static const void* getRecordDataPtr(const configVariableDescriptor_t *descriptor)
{
	/* find the corresponding space in the configuration area */
	const configRecordEntry_t *curRecord = (const configRecordEntry_t *)&_flashConfigROMBegin;
	/* pointer to a memory region to retrieve the configuration data for this descriptor from */
	const void *configDataLoadMemory = descriptor->defaultValuesRegion;

	while (NULL != curRecord &&
			IS_POINTER_IN_CONFIG_MEMORY(curRecord))
	{
		if ((NULL == curRecord->configDescriptorPtr)      || ((void*)~0 == curRecord->configDescriptorPtr)         ||
			(NULL == curRecord->configDataPtr)            || ((void*)~0 == curRecord->configDataPtr))
		{
			/* this is the guard keeper or just an invalid entry (marking the end of all records) */
			/* get out of the loop */
			break;
		} else
		{
			/* test if this is the matching record in the configuration */
			if (curRecord->configDescriptorPtr == descriptor && IS_POINTER_IN_CONFIG_MEMORY(curRecord->configDataPtr))
			{
				configDataLoadMemory = curRecord->configDataPtr;
			}
		}
		++curRecord;
	}

	return configDataLoadMemory;
}

void config_readFromFlash()
{
	const configVariableDescriptor_t *descriptor = &_applicationConfigDescriptorsBegin;
	/* for every descriptor */
	while (descriptor < &_applicationConfigDescriptorsEnd)
	{
		/* pointer to a memory region to retrieve the configuration data for this descriptor from */
		const void *configDataLoadMemory = getRecordDataPtr(descriptor);

		/* retreive the configuration values and put them into the desired space in the RAM */
		if (configDataLoadMemory != NULL)
		{
			memcpy(descriptor->dataPtr, configDataLoadMemory, descriptor->dataLen);
		} else
		{
			/* set to 0 as default */
			memset(descriptor->dataPtr, 0, descriptor->dataLen);
		}

		++descriptor;
	}
}

static void writeWordsToFlash(void *dst, const void *data, uint16_t size)
{
	size = size / 2; /* cnt of half word writes */
	const uint16_t *halfWords = (const uint16_t*) data;
	uint16_t *dstHalfWords = (uint16_t*) dst;
	uint16_t i = 0U;
	for (i = 0U; i < size; ++i)
	{
		flash_program_half_word((uint32_t)&(dstHalfWords[i]), halfWords[i]);
	}
}

bool config_needsUpdate()
{
	bool needsUpdate = false;
	const configVariableDescriptor_t *descriptor = &_applicationConfigDescriptorsBegin;
	while (descriptor < &_applicationConfigDescriptorsEnd)
	{
		const void *configDataLoadMemory = getRecordDataPtr(descriptor);
		if (configDataLoadMemory == NULL || configDataLoadMemory == descriptor->defaultValuesRegion) {
			needsUpdate = true;
			break;
		} else {
			int dataMatch = memcmp(configDataLoadMemory, descriptor->dataPtr, descriptor->dataLen);
			if (0 != dataMatch) {
				needsUpdate = true;
				break;
			}
		}
		++descriptor;
	}
	return needsUpdate;
}

void config_updateToFlash()
{
	bool needsUpdate = config_needsUpdate();

	if (false != needsUpdate)
	{
		setLEDColor(0, 0, 0);
		volatile int i = 10000000;
		while (i --> 0);
		const configVariableDescriptor_t *descriptor = (const configVariableDescriptor_t *)&_applicationConfigDescriptorsBegin;
		const uint16_t descriptorsCnt = ((uint32_t)&_applicationConfigDescriptorsEnd - (uint32_t)&_applicationConfigDescriptorsBegin) / sizeof(configVariableDescriptor_t);
		configRecordEntry_t *dstRecord = (configRecordEntry_t*)&_flashConfigROMBegin;
		uint16_t *dataStoragePtr = (uint16_t*)(dstRecord + descriptorsCnt + 1);

		(void) flash_unlock();
		flash_erase_page(FLASH_CONFIG_START_ADDR);

		/* update the entire list of configuration records */
		while (descriptor < &_applicationConfigDescriptorsEnd)
		{
			configRecordEntry_t record;
			record.configDescriptorPtr = descriptor;
			record.configDataPtr = dataStoragePtr;
			uint32_t actualLen = (descriptor->dataLen + 1) & ~1;

			writeWordsToFlash(dstRecord, &record, sizeof(record));
			writeWordsToFlash(dataStoragePtr, descriptor->dataPtr, actualLen);

			dataStoragePtr += actualLen / 2;
			++dstRecord;
			++descriptor;
		}
		{
			configRecordEntry_t record;
			record.configDescriptorPtr = NULL;
			record.configDataPtr = NULL;
			writeWordsToFlash(dstRecord, &record, sizeof(record));
		}
		flash_lock();
		setLEDColor(255, 0, 0);
	}
}

static void configuration_init(void);
MODULE_INIT_FUNCTION(configuration, 9, configuration_init)
static void configuration_init(void)
{
	/* enable flash interface  clock. */
	config_readFromFlash();
	config_updateToFlash();
}

