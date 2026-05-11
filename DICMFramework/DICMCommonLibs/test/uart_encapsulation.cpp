#include <gtest/gtest.h>

extern "C" {
#include "crc.h"
#include "ddm2.h"
#include "uart_encapsulation.h"
}

#include "DICMFrameworkTestFixture.hpp"

TEST(UARTEncapsulation, test_ddmp2_uart_receive_memcpy_overlap)
{
    const uint8_t data[8] = { DDMP2_CONTROL_GENERIC, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE};
    uint8_t uart_buff[32];
    uint8_t *uart_work_buff = &uart_buff[2];
    uint16_t crc;
    size_t uart_buff_size, uart_work_buff_size = 31;
    DDMP2_FRAME raw_frame, frame;

    EXPECT_EQ(ddmp2_create_raw_frame(&raw_frame, &data, sizeof(data), 0), 1);

    uart_buff_size = ddmp2_uart_bytestuff_frame((void *) &uart_buff[0], &raw_frame);
    crc = crc16_get(&uart_buff[1], uart_buff_size - 2);
    memcpy(&uart_buff[1 + uart_buff_size], &crc, sizeof(crc));
    uart_buff_size += sizeof(crc) + 1;

    EXPECT_EQ(ddmp2_uart_receive(&frame, uart_work_buff, uart_work_buff_size,
                                 uart_buff, uart_buff_size, 0),
               DDMP2_ENCAPSULATION_FRAME);
}
