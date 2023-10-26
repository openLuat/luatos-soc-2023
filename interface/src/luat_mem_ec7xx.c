/*
 * Copyright (c) 2023 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * mem操作
 * 
 */

#include <stdlib.h>
#include "luat_base.h"
#include "common_api.h"
#include "mem_map.h"
#ifdef PSRAM_EXIST
static uint32_t luat_psram_address = PSRAM_START_ADDR;
#endif
#define OSI_ALIGN_UP(v, n) (((unsigned long)(v) + (n)-1) & ~((n)-1))
extern void soc_get_heap_info(uint32_t *total, uint32_t *total_free, uint32_t *min_free);
void* luat_heap_malloc(size_t len) {
    return malloc(len);
}

void luat_heap_free(void* ptr) {
    free(ptr);
}

void* luat_heap_realloc(void* ptr, size_t len) {
    return realloc(ptr, len);
}

void* luat_heap_calloc(size_t count, size_t _size) {
    return calloc(count, _size);
}

void luat_meminfo_sys(size_t *total, size_t *used, size_t *max_used) {
	uint32_t total_free, min_free;
	soc_get_heap_info(total, &total_free, &min_free);
	*used = *total - xPortGetFreeHeapSize();
	*max_used = *total - min_free;
}

void *luat_psram_static_alloc(size_t size)
{
#ifdef PSRAM_EXIST
	uint32_t cr = OS_EnterCritical();
	volatile void *address = NULL;
	size = OSI_ALIGN_UP(size, 8);
	if ((luat_psram_address + size) < PSRAM_END_ADDR)
	{
		address = luat_psram_address;
		luat_psram_address += size;
	}
	OS_ExitCritical(cr);
	return address;
#else
	return NULL;
#endif
}
