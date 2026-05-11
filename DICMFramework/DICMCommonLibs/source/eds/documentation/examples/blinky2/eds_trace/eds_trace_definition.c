/*
 * eds_trace_definition.c
 *
 *  Created on: Dec 28, 2021
 *      Author: nenad
 */

#include "eds_trace.h"

#include <stdio.h>
#include <stdarg.h>

extern void
eds_trace__error(uint32_t source, uint32_t error, const char * format, ...)
{
    va_list args;

    va_start(args, format);
    printf("E::%u::(%u) ", source, error);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

extern void
eds_trace__info(uint32_t source, const char * format, ...)
{
    va_list args;

    printf("I::%u ", source);
    va_start(args, format);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
