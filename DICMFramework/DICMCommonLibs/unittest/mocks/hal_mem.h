
#ifndef HAL_MEM_H_
#define HAL_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define HAL_MEM_SPIRAM 0xdead0001
#define HAL_MEM_INTERNAL_RAM 0xdead0002

void hal_mem_free(void *);
void * hal_mem_malloc_prefer(size_t, int, int);
void * hal_mem_realloc_prefer(void *, size_t, int, int);

#ifdef __cplusplus
}
#endif

#endif
