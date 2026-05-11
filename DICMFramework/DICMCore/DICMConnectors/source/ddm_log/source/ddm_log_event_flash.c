#include "configuration.h"

#include "ddm_log_event.h"
#include "ddm_log_event_flash.h"

#include "ddm_log_event_interface.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_idf_version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "spi_flash_mmap.h"
#endif
#include "esp_attr.h"
#include "esp_partition.h"
#include "esp_flash.h"

#define FIRST_SECTOR                         (0) // flash_s.start / SPI_FLASH_SEC_SIZE )
#define LAST_SECTOR                          ( FIRST_SECTOR + flash_s.nMaxSectors - 1 )
#define SECTOR_NOT_COMMITED                  ( 0xFFFFFFFF )
#define IS_SECTOR_COMMITED(s_hdr)            ( (s_hdr.nEntries == SECTOR_NOT_COMMITED) ? 0 : 1 )
#define IS_VALID_MAGIC_NUM(magic_num)        ( (magic_num) == MAGIC_NUM )
#define IS_VALID_SEC_HDR(hdr)                ( IS_VALID_MAGIC_NUM(hdr.magic_num) )
#define IS_VALID_ENTRY_HDR(hdr)              ( IS_VALID_MAGIC_NUM(((ddm_log_event__data_hdr_t *)hdr)->magic_num) )
#define ABS_SECTOR_POSITION_HDR(sector)      ( ((sector) * flash_s.sectorSize) )
#define ABS_SECTOR_POSITION_DATA(sector)     ( ABS_SECTOR_POSITION_HDR(sector) + sizeof(sector_header_t) )
#define IS_POSITION_WITHIN_BOUNDS(pos)       ( ((pos) >= ABS_SECTOR_POSITION_DATA(FIRST_SECTOR)) && ((pos) <= log_partition->size) )
#define ALL_ENTRIES_IN_ACTIVE_SEC_READ       ( activeReadSector.nSectorEntries == activeReadSector.hdr.nEntries )

typedef memory_interface_t flash_t;

static flash_t flash_s;
static sector_t activeReadSector;

const esp_partition_t *log_partition;

static int ddm_log_event_flash__write(ddm_log_event__descriptor_t *descriptor, void *value, uint32_t size);
static int ddm_log_event_flash__read(ddm_log_event__descriptor_t *descriptor, void *value, uint32_t size, uint8_t type);
static void ddm_log_event_flash__set_read_position(ddm_log_event__descriptor_t *descriptor);
static int ddm_log_event_flash___if_last_entry(ddm_log_event__descriptor_t *descriptor);

static ddm_log_event__descriptor_t flash_descriptor =
{
    .read_pos = 0,
    .write_pos = 0,
    .funcs.ddm_log__read = ddm_log_event_flash__read,
    .funcs.ddm_log__write = ddm_log_event_flash__write,
    .funcs.ddm_log__set_read_position = ddm_log_event_flash__set_read_position,
    .funcs.ddm_log__all_entries_read = ddm_log_event_flash___if_last_entry
};

static uint32_t next_sector(uint32_t sector)
{
    return (sector >= LAST_SECTOR ? FIRST_SECTOR : (sector + 1));
}

static uint32_t enough_space_to_end_of_sector(const uint32_t sector, const uint32_t pos, const uint32_t size)
{
    return (pos + size) <= (ABS_SECTOR_POSITION_HDR(sector + 1));
}

static void commit_sector(uint32_t sector, uint32_t nEntries)
{
    sector_header_t s_hdr = { .nEntries = nEntries, .magic_num = MAGIC_NUM };
    
    esp_partition_write(log_partition, ABS_SECTOR_POSITION_HDR(sector), &s_hdr, sizeof(sector_header_t));
}

static uint32_t last_event_in_sector_pos(const uint32_t last_written_sector)
{
    uint32_t pos = ABS_SECTOR_POSITION_DATA(last_written_sector);

    while (enough_space_to_end_of_sector(last_written_sector, pos, ENTRY_HEADER_SIZE))
    {
        struct ddm_log_event__data last_writen_entry_hdr;
        esp_partition_read(log_partition, pos, &last_writen_entry_hdr.val.header, sizeof(ddm_log_event__data_hdr_t));

        if (!IS_VALID_ENTRY_HDR(&last_writen_entry_hdr.val.header))
        {
            char hdr[sizeof(ddm_log_event__data_hdr_t)];
            memset(hdr, 0xFF, sizeof(ddm_log_event__data_hdr_t));

            // If not FF, indicates data corruption
            // Rewrite the sector, while removing the last invalid entry
            if (memcmp(&last_writen_entry_hdr, hdr, sizeof(hdr)) != 0)
            {
                uint32_t rewriteDataLen = pos - ABS_SECTOR_POSITION_HDR(last_written_sector);

                char * rewriteData = heap_caps_malloc_prefer(rewriteDataLen * sizeof(char), 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
                if (rewriteData)
                {
                    esp_partition_read(log_partition, ABS_SECTOR_POSITION_HDR(last_written_sector), rewriteData, rewriteDataLen);
                    esp_partition_erase_range(log_partition, ABS_SECTOR_POSITION_HDR(last_written_sector), flash_s.sectorSize);
                    esp_partition_write(log_partition, ABS_SECTOR_POSITION_HDR(last_written_sector), rewriteData, rewriteDataLen);

                    heap_caps_free(rewriteData);
                }
            }

            break;
        }

        pos += ENTRY_SIZE(&last_writen_entry_hdr);
        flash_s.nWriteSectorEntries++;
    }

    return pos;
}

static void set_read_position(ddm_log_event__descriptor_t *descriptor, uint32_t sector)
{
    sector_header_t s_header;

    activeReadSector.nSectorEntries = 0;
    activeReadSector.sector = sector;

    esp_partition_read(log_partition, ABS_SECTOR_POSITION_HDR(sector), &s_header, sizeof(sector_header_t));
    activeReadSector.hdr.nEntries = s_header.nEntries;
    activeReadSector.hdr.magic_num = s_header.magic_num;

    descriptor->read_pos = ABS_SECTOR_POSITION_DATA(sector);
}

static int ddm_log_event_flash__scan(void)
{
    uint32_t firstEmptySec = 0;
    uint32_t lastEmptySec = 0;
    uint32_t nSectorsNotCommited = 0;

    for (uint32_t sector = FIRST_SECTOR; sector <= LAST_SECTOR; ++sector)
    {
        sector_header_t s_header;
        esp_partition_read(log_partition, ABS_SECTOR_POSITION_HDR(sector), &s_header, sizeof(sector_header_t));

        if (!IS_SECTOR_COMMITED(s_header)) // empty of currenty writting
        {
            ++nSectorsNotCommited;
            if (nSectorsNotCommited == 1)
            {
                firstEmptySec = sector;
            }
            else
            {
                lastEmptySec = sector;
            }
        }
    }

    if (nSectorsNotCommited == 0)
    {
        LOG(E, "Something is wrong, nSectorsNotCommited[%d]", nSectorsNotCommited);
        return 0;
    }
    else if (nSectorsNotCommited == 1)
    {
        flash_s.readSector = next_sector(firstEmptySec);
        flash_s.writeSector = firstEmptySec;
    }
    else // if more then one is not commited, fist time writing in memory or power failure while commiting sector
    {
        if (nSectorsNotCommited == 2)
        {
            if ((firstEmptySec == FIRST_SECTOR) && (lastEmptySec == LAST_SECTOR))
            {
                flash_s.readSector = next_sector(firstEmptySec);
                flash_s.writeSector = lastEmptySec;
            }
            else
            {
                flash_s.readSector = next_sector(lastEmptySec);
                flash_s.writeSector = firstEmptySec;
            }
        }
        else
        {
            flash_s.readSector = FIRST_SECTOR;
            flash_s.writeSector = firstEmptySec;
        }
    }

    flash_descriptor.write_pos = last_event_in_sector_pos(flash_s.writeSector);
    ddm_log_event_flash__set_read_position(&flash_descriptor);

    return 1;
}

static int read_header(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size)
{
    if (IS_SECTOR_COMMITED(activeReadSector.hdr))
    {
        if (IS_VALID_SEC_HDR(activeReadSector.hdr))
        {
            if (ALL_ENTRIES_IN_ACTIVE_SEC_READ)
            {
                set_read_position(descriptor, next_sector(activeReadSector.sector));
                if (ddm_log_event_flash___if_last_entry(descriptor))
                {
                    return 0;
                }
            }
        }
        else
        {
            if (activeReadSector.sector != flash_s.writeSector)
            {
                // skip current sector due to data corruptions of sector's header
                set_read_position(descriptor, next_sector(activeReadSector.sector));
                if (ddm_log_event_flash___if_last_entry(descriptor))
                {
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
    }

    if (!IS_POSITION_WITHIN_BOUNDS(descriptor->read_pos + size))
    {
        //LOG(E, "descriptor->write_pos + size [%d] > ram_s.end[%d]\n", descriptor->read_pos + size, flash_s.end);
        return 0;
    }

    esp_partition_read(log_partition, descriptor->read_pos, value, size);

    if (!IS_VALID_ENTRY_HDR(value))
    {
        //LOG(E, "IS_VALID_ENTRY_HDR[%d] MAGIC_NUM[%d]\n", IS_VALID_ENTRY_HDR(value), ((ddm_log_event__data_hdr_t *)value)->magic_num);
        return 0;
    }

    activeReadSector.nSectorEntries++;

    return 1;
}

static int read_value(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size)
{
    if (!IS_POSITION_WITHIN_BOUNDS(descriptor->read_pos + size))
    {
        //LOG(E, "descriptor->write_pos + size [%d] > ram_s.end[%d]\n", descriptor->read_pos + size, flash_s.end);
        return 0;
    }
    esp_partition_read(log_partition, descriptor->read_pos, value, size);

    return 1;
}

static int ddm_log_event_flash__read(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size, uint8_t type)
{
    uint32_t validRead = 0;

    if (size > flash_s.sectorSize)
    {
        return validRead;
    }

    if (type == ENTRY_HEADER)
    {
        validRead = read_header(descriptor, value, size);
    }
    else if (type == ENTRY_VALUE)
    {
        validRead = read_value(descriptor, value, size);
    }

    return validRead;
}

static void addvance_to_next_sector(ddm_log_event__descriptor_t *descriptor)
{
    uint32_t nextWriteSector = next_sector(flash_s.writeSector);
    descriptor->write_pos = ABS_SECTOR_POSITION_DATA(nextWriteSector);

    esp_partition_erase_range(log_partition, descriptor->write_pos, flash_s.sectorSize);

    commit_sector(flash_s.writeSector, flash_s.nWriteSectorEntries);
    flash_s.writeSector = nextWriteSector;
    flash_s.nWriteSectorEntries = 0;

    // move to next sector, if overriding
    if (ddm_log_event_flash___if_last_entry(descriptor))
    {
        flash_s.readSector = next_sector(flash_s.writeSector);
    }

    set_read_position(descriptor, flash_s.readSector);
}

static int ddm_log_event_flash__write(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size)
{
    if (size > flash_s.sectorSize)
    {
        return 0;
    }

    if (!enough_space_to_end_of_sector(flash_s.writeSector, descriptor->write_pos, size))
    {
        addvance_to_next_sector(descriptor);
    }

    if (!IS_POSITION_WITHIN_BOUNDS(descriptor->write_pos + size))
    {
        return 0;
    }
    
    esp_partition_write(log_partition, descriptor->write_pos, value, size);
    flash_s.nWriteSectorEntries++;

    return 1;
}

static void ddm_log_event_flash__set_read_position(ddm_log_event__descriptor_t *descriptor)
{
    set_read_position(descriptor, flash_s.readSector);
}

static int ddm_log_event_flash___if_last_entry(ddm_log_event__descriptor_t *descriptor)
{
    return (descriptor->write_pos == descriptor->read_pos ? 1 : 0);
}

ddm_log_event__descriptor_t *ddm_log_event_flash__init(uint32_t *size)
{
    (void)size;

    if ((log_partition = get_log_partition()))
    {
        if ((log_partition->address % SPI_FLASH_SEC_SIZE != 0) || (log_partition->size % SPI_FLASH_SEC_SIZE != 0))
        {
            LOG(E, "'%s' partition not aligned.", get_log_partition_label());
            return NULL;
        }

        flash_s.start = log_partition->address;
        flash_s.end = log_partition->address + log_partition->size;

        flash_s.sectorSize = SPI_FLASH_SEC_SIZE;
        flash_s.nMaxSectors = log_partition->size / SPI_FLASH_SEC_SIZE; //(flash_s.end - flash_s.start) / flash_s.sectorSize;

        if (!ddm_log_event_flash__scan())
        {
            return NULL;
        }
    }
    else
    {
        LOG(E, "Partition table set wrong, no '%s' partition exists.", get_log_partition_label());
        return NULL;
    }

    return &flash_descriptor;
}
