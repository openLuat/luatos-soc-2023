/*
 * FreeRTOS Kernel V9.0.0a
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>
#include <string.h>//add for memset

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#if( configSUPPORT_DYNAMIC_ALLOC_HEAP == 4 )
#include "task.h"
#include "mm_debug.h"//add for memory leak debug
#include "exception_process.h"
#include "cmsis_compiler.h"
#include "mem_map.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

// add 8 bytes to record footprint for each allocated memory block
//the footprint value should never be modified
#define MALLOC_FOOT_PRINT_SIZE      ( ( size_t ) 8 )


/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE  ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE       ( ( size_t ) 8 )

#ifdef CORE_IS_AP
    extern UINT32 Image$$LOAD_DRAM_SHARED$$ZI$$Limit;
#endif


/* Allocate the memory for the heap. */
#if( configAPPLICATION_ALLOCATED_HEAP == 1 )
    /* The application writer has already defined the array used for the RTOS
    heap - probably so it can be placed in a special segment or address. */
    extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
    //static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
    #ifdef CORE_IS_AP
    uint8_t * ucHeap=(uint8_t *)&( Image$$LOAD_DRAM_SHARED$$ZI$$Limit);
    #else
    static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];//cp still use fix length array
    #endif


//dynamic heap size, caculate per compilation
UINT32 gTotalHeapSize=0;

#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock;   /*<< The next free block in the list. */
    size_t xBlockSize;/*<< The size of the free block. */
    size_t xBlockRealSize;/*<< The real size of the malloc block. add for realloc, becase malloc 255 and 256, the xBlockSize is the same value*/
    #ifdef MM_DEBUG_EN
    void * allocateOwner;
    #endif
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;


#ifdef HEAP_MEM_DEBUG
void *recThreadId[50];
int thIdx = 0;
extern void * osThreadGetId (void);
/*-----------------------------------------------------------*/
#endif

void *pvPortZeroMalloc( size_t xWantedSize)
{
    void *ptr = pvPortMallocEC(xWantedSize, __GET_RETURN_ADDRESS());
    return ptr ? memset(ptr, 0, xWantedSize), ptr : ptr;
}

void *pvPortAssertMalloc( size_t xWantedSize)
{
    void *ptr = pvPortMallocEC(xWantedSize, __GET_RETURN_ADDRESS());
    configASSERT(ptr != 0);
    return ptr;
}

void *pvPortZeroAssertMalloc( size_t xWantedSize)
{
    void *ptr = pvPortMallocEC(xWantedSize, __GET_RETURN_ADDRESS());
    configASSERT(ptr != 0);
    memset(ptr, 0, xWantedSize);
    return ptr;
}

void *pvPortMallocEC( size_t xWantedSize, unsigned int funcPtr )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;
size_t realSize=0;
size_t xWantedSizeTemp=xWantedSize;

#ifdef HEAP_MEM_DEBUG
BlockLink_t *pxBlockDbg;
#endif
    configASSERT(__get_IPSR()==0);

    configASSERT(xWantedSize > 0);

    vTaskSuspendAll();
    {
        /* If this is the first call to malloc then the heap will require
        initialisation to setup the list of free blocks. */
        if( pxEnd == NULL )
        {
            #ifdef MM_DEBUG_EN
            mm_trace_init();
            #endif

            prvHeapInit();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        #ifdef HEAP_MEM_DEBUG
        pxBlockDbg = xStart.pxNextFreeBlock;
        while( ( pxBlockDbg->pxNextFreeBlock != NULL ) )
        {
            pxBlockDbg = pxBlockDbg->pxNextFreeBlock;
        }
        if(pxBlockDbg != pxEnd)
        {
            configASSERT(FALSE);
        }
        #endif


        /* Check the requested block size is not so large that the top bit is
        set.  The top bit of the block size member of the BlockLink_t structure
        is used to determine who owns the block - the application or the
        kernel, so it must be free. */
        if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
        {
            /* The wanted size must be increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. */
            if( ( xWantedSize > 0 ) && 
                ( ( xWantedSize + xHeapStructSize ) >  xWantedSize ) ) /* Overflow check */
            {
                xWantedSize += xHeapStructSize;
                #ifdef MM_DEBUG_EN
                xWantedSize += MALLOC_FOOT_PRINT_SIZE;// malloc additional 8 bytes at the end
                #endif
                /* Ensure that blocks are always aligned. */
                of bytes. */
                if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
                {
                    /* Byte alignment required. Check for overflow. */
                    if( ( xWantedSize + ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) ) ) 
                            > xWantedSize )
                    {
                        xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
                        configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
                    }
                    else
                    {
                        xWantedSize = 0;
                    }  
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                xWantedSize = 0;
            }

            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
            {
                /* Traverse the list from the start (lowest address) block until
                one of adequate size is found. */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;
                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                was not found. */
                if( pxBlock != pxEnd )
                {
                    /* Return the memory space pointed to - jumping over the
                    BlockLink_t structure at its start. */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    /* This block is being returned for use so must be taken out
                    of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                    two. */
                    if(pxBlock->xBlockSize < xWantedSize)
                    {
                        configASSERT(FALSE);
                    }
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /* This block is to be split into two.  Create a new
                        block following the number of bytes requested. The void
                        cast is used to prevent byte alignment warnings from the
                        compiler. */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
                        configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

                        /* Calculate the sizes of two blocks split from the
                        single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList( pxNewBlockLink );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
                    {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                    by the application and has no "next" block. */
                    realSize=pxBlock->xBlockSize;
                    pxBlock->xBlockRealSize = xWantedSizeTemp;
                    pxBlock->xBlockSize |= xBlockAllocatedBit;
                    pxBlock->pxNextFreeBlock = NULL;
                    #ifdef MM_DEBUG_EN
                    pxBlock->allocateOwner= osThreadGetId();

                    /*set the footprint*/
                    *(uint32_t *)((uint8_t*)pxBlock+realSize-MALLOC_FOOT_PRINT_SIZE)=0xdeadbeaf;
                    *(uint32_t *)((uint8_t*)pxBlock+realSize-MALLOC_FOOT_PRINT_SIZE+4)=0xdeadbeaf;
                    #endif
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xWantedSize );
    }

    #ifdef HEAP_MEM_DEBUG
    pxBlockDbg = xStart.pxNextFreeBlock;
    while ((pxBlockDbg->pxNextFreeBlock != NULL))
    {
        pxBlockDbg = pxBlockDbg->pxNextFreeBlock;
    }
    if(pxBlockDbg != pxEnd)
    {
        configASSERT(FALSE);
    }
    #endif

    #ifdef MM_DEBUG_EN
    if( pvReturn != NULL )
    {
        if(funcPtr == 0)
        {
            funcPtr = __GET_RETURN_ADDRESS();
        }

        mm_malloc_trace(pvReturn, xWantedSize, funcPtr);
    }
    #endif

    ( void ) xTaskResumeAll();

    #if( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
        if( pvReturn == NULL )
        {
            extern void vApplicationMallocFailedHook( void );
            vApplicationMallocFailedHook();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif

    configASSERT( pvReturn != 0 );
    configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );

    return pvReturn;
}

/*-----------------------------------------------------------*/
void vPortFree( void *pv )
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;
#ifdef HEAP_MEM_DEBUG
BlockLink_t *pxBlockDbg;
#endif
    configASSERT(__get_IPSR()==0);
    configASSERT( pv != NULL );

    //ucHeap
    configASSERT( (uint32_t)pv >= (uint32_t)ucHeap && (uint32_t)pv < ((uint32_t)ucHeap) + gTotalHeapSize );

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
        before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        //EC_ASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit, 0, 0, 0 ) != 0 );

        /* Check the block is actually allocated. */
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /* The block is being returned to the heap - it is no longer
                allocated. */
                pxLink->xBlockSize &= ~xBlockAllocatedBit;
                #ifdef MM_DEBUG_EN
                /*check the memory block's footprint*/
                configASSERT(*(uint32_t *)((uint8_t*)pxLink+pxLink->xBlockSize-MALLOC_FOOT_PRINT_SIZE)== 0xdeadbeaf );
                configASSERT(*(uint32_t *)((uint8_t*)pxLink+pxLink->xBlockSize-MALLOC_FOOT_PRINT_SIZE+4) == 0xdeadbeaf );
                #endif

                vTaskSuspendAll();
                #ifdef HEAP_MEM_DEBUG
                pxBlockDbg = xStart.pxNextFreeBlock;
                while ((pxBlockDbg->pxNextFreeBlock != NULL))
                {
                    pxBlockDbg = pxBlockDbg->pxNextFreeBlock;
                }
                if(pxBlockDbg != pxEnd)
                {
                    configASSERT(FALSE);
                }
                #endif
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    configASSERT(pxLink->xBlockSize > 0 && pxLink->xBlockSize < gTotalHeapSize);

                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );

                    #ifdef MM_DEBUG_EN
                    mm_free_trace(pv);
                    #endif
                }
                #ifdef HEAP_MEM_DEBUG
                pxBlockDbg = xStart.pxNextFreeBlock;
                while ((pxBlockDbg->pxNextFreeBlock != NULL))
                {
                    pxBlockDbg = pxBlockDbg->pxNextFreeBlock;
                }
                if(pxBlockDbg != pxEnd)
                {
                    configASSERT(FALSE);
                }
                #endif
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

    }
}

/*----add for libcoap-------------------------------------------------------*/
void *pvPortRealloc( void *pv, size_t xWantedSize )
{
    uint8_t *puc = ( uint8_t * ) pv;
    void *newPuc = NULL;
    BlockLink_t *pxLink;

    if(xWantedSize == 0)
    {
        return NULL;
    }

    if( puc != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );
        if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
        {
            if((pxLink->xBlockRealSize) >= xWantedSize) /* if malloc(255), and realloc(old, 256), the size is the same*/
            {
                return pv;
            }
            else
            {
                newPuc = pvPortMalloc(xWantedSize);
                if(newPuc != NULL)
                {
                    memset(newPuc, 0, xWantedSize);
                    memcpy(newPuc, pv, pxLink->xBlockRealSize);
                    vPortFree(pv);
                    return newPuc;
                }else
                {
                    return NULL;
                }
            }
        }
    }
    return NULL;
}

size_t xPortGetTotalHeapSize( void )
{
    return gTotalHeapSize;
}

/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void )
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;
    size_t uxAddress;
    size_t xTotalHeapSize;
#ifdef CORE_IS_AP
    gTotalHeapSize = MSMB_APMEM_END_ADDR - (UINT32)&( Image$$LOAD_DRAM_SHARED$$ZI$$Limit);
#else
    gTotalHeapSize = configTOTAL_HEAP_SIZE;
#endif
    xTotalHeapSize = gTotalHeapSize;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = ( size_t ) ucHeap;

    if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxAddress += ( portBYTE_ALIGNMENT - 1 );
        uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
    }

    pucAlignedHeap = ( uint8_t * ) uxAddress;

    /* xStart is used to hold a pointer to the first item in the list of free
    blocks.  The void cast is used to prevent compiler warnings. */
    xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart.xBlockSize = ( size_t ) 0;

    /* pxEnd is used to mark the end of the list of free blocks and is inserted
    at the end of the heap space. */
    uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
    pxEnd = ( void * ) uxAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that is sized to take up the
    entire heap space, minus the space taken by pxEnd. */
    pxFirstFreeBlock = ( void * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

    /* Iterate through the list until a block is found that has a higher address
    than the block being inserted. */
    for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
    make a contiguous block of memory? */
    puc = ( uint8_t * ) pxIterator;
    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
    make a contiguous block of memory? */
    puc = ( uint8_t * ) pxBlockToInsert;
    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
    {
        if( pxIterator->pxNextFreeBlock != pxEnd )
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd;
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
    before and the block after, then it's pxNextFreeBlock pointer will have
    already been set, and should not be set here as that would make it point
    to itself. */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
#endif

