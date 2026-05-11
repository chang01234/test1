/*
 * ddm_log_event_interface.h
 *
 *  Created on: 25 apr. 2024
 *      Author: Andlun
 */

#ifndef DDM_LOG_EVENT_INTERFACE_H_
#define DDM_LOG_EVENT_INTERFACE_H_

#include <stdint.h>

typedef struct sector_header
{
    uint32_t magic_num;
    uint32_t nEntries;
} sector_header_t;

typedef struct sector
{
    sector_header_t hdr;
    uint32_t sector;
    uint32_t nSectorEntries;
} sector_t;

typedef struct memory_interface
{
    uint32_t start;
    uint32_t end;
    uint32_t sectorSize;
    uint32_t nMaxSectors;
    uint32_t writeSector;
    uint32_t readSector;
    uint32_t nWriteSectorEntries;
} memory_interface_t;

#endif /* DDM_LOG_EVENT_INTERFACE_H_ */
