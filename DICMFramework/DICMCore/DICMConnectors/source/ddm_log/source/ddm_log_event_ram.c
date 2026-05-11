#include <string.h>

#include "configuration.h"

#include "ddm_log_event.h"
#include "ddm_log_event_ram.h"

#include "ddm_log_event_interface.h"

#ifndef DDM_LOG_RAM_BUFFER_SIZE

#define RAM_BUFFER_SIZE                      1024
#else
#define RAM_BUFFER_SIZE                      DDM_LOG_RAM_BUFFER_SIZE
#endif

#define RAM_SEC_SIZE                         1024
#define FIRST_SECTOR                         ( ram_s.start / RAM_SEC_SIZE )
#define SECTOR_NOT_COMMITED                  ( 0 )
#define IS_SECTOR_COMMITED(s_hdr)            ( (s_hdr.nEntries == SECTOR_NOT_COMMITED) ? 0 : 1 )
#define IS_VALID_MAGIC_NUM(magic_num)        ( (magic_num) == MAGIC_NUM )
#define IS_VALID_SEC_HDR(hdr)                ( IS_VALID_MAGIC_NUM(hdr.magic_num) )
#define IS_VALID_ENTRY_HDR(hdr)              ( IS_VALID_MAGIC_NUM(((ddm_log_event__data_hdr_t *)hdr)->magic_num) )
#define ABS_SECTOR_POSITION_HDR(sector)      ( ((sector) * ram_s.sectorSize) )
#define ABS_SECTOR_POSITION_DATA(sector)     ( ABS_SECTOR_POSITION_HDR(sector) + sizeof(sector_header_t) )
#define IS_POSITION_WITHIN_BOUNDS(pos)       ( ((pos) >= ABS_SECTOR_POSITION_DATA(FIRST_SECTOR)) && ((pos) <= ram_s.end ) )
#define ALL_ENTRIES_IN_ACTIVE_SEC_READ       ( activeReadSector.nSectorEntries == activeReadSector.hdr.nEntries )

typedef memory_interface_t ram_t;

static ram_t ram_s;
static sector_t activeReadSector;
static char *buffer = NULL;

static int ddm_log_event_ram__write(ddm_log_event__descriptor_t *descriptor, void *value, uint32_t size);
static int ddm_log_event_ram__read(ddm_log_event__descriptor_t *descriptor, void *value, uint32_t size, uint8_t type);
static void ddm_log_event_ram__set_read_position(ddm_log_event__descriptor_t *descriptor);
static int ddm_log_event_ram___if_last_entry(ddm_log_event__descriptor_t *descriptor);


static ddm_log_event__descriptor_t ram_descriptor =
{
    .funcs.ddm_log__read = ddm_log_event_ram__read,
    .funcs.ddm_log__write = ddm_log_event_ram__write,
    .funcs.ddm_log__set_read_position = ddm_log_event_ram__set_read_position,
    .funcs.ddm_log__all_entries_read = ddm_log_event_ram___if_last_entry
};

static uint32_t next_sector(const uint32_t sector)
{
    return (sector >= (ram_s.nMaxSectors - 1) ? FIRST_SECTOR : (sector + 1));
}

static uint32_t enough_space_to_end_of_sector(uint32_t sector, uint32_t pos, uint32_t size)
{
    return ((pos + size) <= (ABS_SECTOR_POSITION_HDR(sector) + RAM_SEC_SIZE));
}

static void commit_sector(uint32_t sector, uint32_t nEntries)
{
    sector_header_t s_hdr = { .nEntries = nEntries, .magic_num = MAGIC_NUM };
    memcpy(&buffer[ABS_SECTOR_POSITION_HDR(sector)], &s_hdr, sizeof(sector_header_t));
}

static void set_read_position(ddm_log_event__descriptor_t *descriptor, uint32_t sector)
{
    sector_header_t s_header;

    activeReadSector.nSectorEntries = 0;
    activeReadSector.sector = sector;

    memcpy(&s_header, &buffer[ABS_SECTOR_POSITION_HDR(sector)], sizeof(sector_header_t));
    activeReadSector.hdr.nEntries = s_header.nEntries;
    activeReadSector.hdr.magic_num = s_header.magic_num;

    descriptor->read_pos = ABS_SECTOR_POSITION_DATA(sector);
}

static uint32_t allocate_ram_buffer(uint32_t size)
{
    uint32_t buffer_size = RAM_BUFFER_SIZE;

    if (size / RAM_SEC_SIZE > 0)
    {
        buffer_size = (size / RAM_SEC_SIZE) * RAM_SEC_SIZE;
    }

    if (buffer)
    {
        // do not reallocate if the requsted size
        if (buffer_size == (ram_s.end - ram_s.start))
        {
            return buffer_size;
        }

        free(buffer);
    }

    buffer = malloc(buffer_size);
    if (!buffer)
    {
        //LOG(E, "Allocating size RAM_BUFFER !buffer[%d]\n", buffer_size);
        return 0;
    }

    memset(buffer, 0, buffer_size);

    return buffer_size;
}

static int read_header(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size)
{
    if (IS_SECTOR_COMMITED(activeReadSector.hdr))
    {
        if (IS_VALID_SEC_HDR(activeReadSector.hdr))
        {
            if (ALL_ENTRIES_IN_ACTIVE_SEC_READ)
            {
                if (ram_s.nMaxSectors > 1) set_read_position(descriptor, next_sector(activeReadSector.sector));
                if (ddm_log_event_ram___if_last_entry(descriptor))
                {
                    return 0;
                }
            }
        }
        else
        {
            //LOG(E,  "Ram_read: sector commited, but not valid");
            return 0;
        }
    }

    if (!IS_POSITION_WITHIN_BOUNDS(descriptor->read_pos + size))
    {
        //LOG(E,  "Ram_read: IS_POSITION_WITHIN_BOUNDS[%d]> ram_s.end[%d]\n", descriptor->read_pos + size, ram_s.end);
        return 0;
    }

    memcpy(value, &buffer[descriptor->read_pos], size);

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
        //LOG(E,  "Ram_read: IS_POSITION_WITHIN_BOUNDS[%d] > ram_s.end[%d]\n", descriptor->read_pos + size, ram_s.end);
        return 0;
    }

    memcpy(value, &buffer[descriptor->read_pos], size);

    return 1;
}

static int ddm_log_event_ram__read(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size, uint8_t type)
{
    uint32_t validRead = 0;

    if (size > ram_s.sectorSize)
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
    uint32_t nextWriteSector = next_sector(ram_s.writeSector);
    descriptor->write_pos = ABS_SECTOR_POSITION_DATA(nextWriteSector);

    // erase sector
    memset(&buffer[ABS_SECTOR_POSITION_HDR(nextWriteSector)], 0, RAM_SEC_SIZE);

    // commit last modified sector
    commit_sector(ram_s.writeSector, ram_s.nWriteSectorEntries);
    ram_s.writeSector = nextWriteSector;
    ram_s.nWriteSectorEntries = 0;

    // move to next sector, if overriding
    if (ddm_log_event_ram___if_last_entry(descriptor))
    {
        LOG(W,  "descriptor->write_pos[%d] == descriptor->read_pos[%d]\n", descriptor->write_pos, descriptor->read_pos);
        ram_s.readSector = next_sector(ram_s.writeSector);
    }

    set_read_position(descriptor, ram_s.readSector);
}

static int ddm_log_event_ram__write(ddm_log_event__descriptor_t *descriptor, void * value, uint32_t size)
{
    if (size > ram_s.sectorSize)
    {
        return 0;
    }

    if (!enough_space_to_end_of_sector(ram_s.writeSector, descriptor->write_pos, size))
    {
       addvance_to_next_sector(descriptor);
    }

    if (!IS_POSITION_WITHIN_BOUNDS(descriptor->write_pos + size))
    {
        //LOG(E, "IS_POSITION_WITHIN_BOUNDS [%d]> ram_s.end[%d]\n", descriptor->write_pos + size, ram_s.end);
        return 0;
    }

    memcpy(&buffer[descriptor->write_pos], value, size);
    ram_s.nWriteSectorEntries++;

    return 1;
}

static void ddm_log_event_ram__set_read_position(ddm_log_event__descriptor_t *descriptor)
{
    set_read_position(descriptor, ram_s.readSector);
}

static int ddm_log_event_ram___if_last_entry(ddm_log_event__descriptor_t *descriptor)
{
    return (descriptor->write_pos == descriptor->read_pos ? 1 : 0);
}

ddm_log_event__descriptor_t *ddm_log_event_ram__init(uint32_t *size)
{
    uint32_t buffer_size = 0;

    buffer_size = allocate_ram_buffer(*size);
    if (!buffer_size)
    {
        *size = 0;
        return NULL;
    }

    ram_s.start = 0;
    ram_s.end = buffer_size;

    ram_s.sectorSize = RAM_SEC_SIZE;
    ram_s.nMaxSectors = (ram_s.end - ram_s.start) / ram_s.sectorSize;
    ram_s.readSector = 0;
    ram_s.writeSector = 0;
    ram_s.nWriteSectorEntries = 0;

    memset(&activeReadSector, 0, sizeof(activeReadSector));

    ram_descriptor.write_pos = ram_descriptor.read_pos = ABS_SECTOR_POSITION_DATA(FIRST_SECTOR);

    *size = buffer_size;
    return &ram_descriptor;
}
