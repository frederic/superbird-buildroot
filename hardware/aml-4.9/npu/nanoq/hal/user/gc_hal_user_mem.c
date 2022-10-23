/****************************************************************************
*
*    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


#include "gc_hal_user_precomp.h"

#if (gcdENABLE_3D)

#include "gc_hal_mem.h"

#define _GC_OBJ_ZONE    gcdZONE_MEM

/******************************************************************************\
|************************* Memory management Functions ************************|
|******************************************************************************|
|* There are only two types of memory pools here:                             *|
|*   - Fixed size node for linked list                                        *|
|*   - Fixed size node array                                                  *|
|* Names are not used in optimizer, so there is no need for variable size     *|
|* memory pool.                                                               *|
|* The implementation is specialized for optimizer to improve performance.    *|
\******************************************************************************/

#define NEW_MEMPOOL     1

/*******************************************************************************
**                          Memory Pool Constants
*******************************************************************************/

/* Currently, the maximal block size is 16MB. */
#define _BLOCKSIZE_TABLE_SIZE           8
static const gctUINT    _blockSize[_BLOCKSIZE_TABLE_SIZE + 1] = {
        256,       1024,       4096,       16384,
      65536,     262144,    1048576,     4194304,
   16777216
};

#define _WORD_SIZE                      4
#define _VS_FREE_LIST_ARRAY_SIZE        16

/*******************************************************************************
**                          Memory Pool Data Structures
*******************************************************************************/
typedef struct _gcsMEM_BLOCK *          gcsMEM_BLOCK;
typedef struct _gcsMEM_FS_MEM_NODE *    gcsMEM_FS_MEM_NODE;
typedef struct _gcsMEM_VS_MEM_NODE *    gcsMEM_VS_MEM_NODE;
typedef struct _gcsMEM_AFS_MEM_NODE *   gcsMEM_AFS_MEM_NODE;

struct _gcsMEM_BLOCK
{
    /* Next block. */
    gcsMEM_BLOCK            next;

    /* Data area - should not be accessed. */
    gctPOINTER              data;
};

struct _gcsMEM_FS_MEM_NODE
{
    /* Next memory node. */
    gcsMEM_FS_MEM_NODE      next;

    /* Data area - should not be accessed. */
    gctPOINTER              data;
};

struct _gcsMEM_FS_MEM_POOL
{
    /* Allocated block list. */
    gcsMEM_BLOCK            blockList;

    /* Free node list. */
    gcsMEM_FS_MEM_NODE      freeList;

    /* Number of nodes in each memory block. */
    gctUINT                 nodeCount;

    /* Node size. */
    gctUINT                 nodeSize;

    /* Block size. */
    gctUINT                 blockSize;
};

struct _gcsMEM_VS_MEM_NODE
{
    /* Node size. */
    gctUINT                 sizeIndex;

    /* Next memory node. */
    gcsMEM_VS_MEM_NODE      next;

    /* Data area - should not be accessed. */
    gctPOINTER              data;
};

struct _gcsMEM_VS_MEM_POOL
{
    /* Allocated block list. */
    gcsMEM_BLOCK            blockList;

    /* Free node list array. */
    gcsMEM_VS_MEM_NODE      freeListArray[_VS_FREE_LIST_ARRAY_SIZE + 1];

    /* Block size. */
    gctUINT                 blockSize;

    /* Size of the free part of the block. */
    gctUINT                 freeSize;

    /* Pointer to free area. */
    gctUINT8_PTR            freeData;

    /* Wheter to recycle free node--implied extra field in node to save size. */
    gctBOOL                 recycleFreeNode;
};

struct _gcsMEM_AFS_MEM_NODE
{
    /* Previous memory array node. */
    gcsMEM_AFS_MEM_NODE     prev;

    /* Next memory array node. */
    gcsMEM_AFS_MEM_NODE     next;

    /* Number of nodes in each memory block. */
    gctUINT                 nodeCount;

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    /* Is freed. */
    gctBOOL                 isFreed;
#endif

    /* Data area - should not be accessed. */
    gctPOINTER              data;
};

struct _gcsMEM_AFS_MEM_POOL
{
    /* Allocated block list. */
#if NEW_MEMPOOL
    gcsMEM_BLOCK            blockList;
#else
    gcsMEM_AFS_MEM_NODE     blockList;
#endif

    /* Free node list. */
    gcsMEM_AFS_MEM_NODE     freeList;

    /* Number of nodes in each memory block. */
    gctUINT                 nodeCount;

    /* Node size. */
    gctUINT                 nodeSize;

    /* Block size. */
    gctUINT                 blockSize;

    /* Size of the free part of the block. */
    gctUINT                 freeSize;

    /* Pointer to free area. */
    gctUINT8_PTR            freeData;
};

#ifdef __GNUC__
#define BLOCK_DATA_OFFSET               (gctUINT) (gctSIZE_T) &(((gcsMEM_BLOCK)0)->data)
#define FS_MEM_NODE_DATA_OFFSET         (gctUINT) (gctSIZE_T) &(((gcsMEM_FS_MEM_NODE)0)->data)
#define VS_MEM_NODE_DATA_OFFSET         (gctUINT) (gctSIZE_T) &(((gcsMEM_VS_MEM_NODE)0)->next)
#define AFS_MEM_NODE_DATA_OFFSET        (gctUINT) (gctSIZE_T) &(((gcsMEM_AFS_MEM_NODE)0)->data)
#else
#ifdef UNDER_CE
#define BLOCK_DATA_OFFSET               (gctUINT) (gctSIZE_T) &(((gcsMEM_BLOCK)0)->data)
#define FS_MEM_NODE_DATA_OFFSET         (gctUINT) (gctSIZE_T) &(((gcsMEM_FS_MEM_NODE)0)->data)
#define VS_MEM_NODE_DATA_OFFSET         (gctUINT) (gctSIZE_T) &(((gcsMEM_VS_MEM_NODE)0)->next)
#define AFS_MEM_NODE_DATA_OFFSET        (gctUINT) (gctSIZE_T) &(((gcsMEM_AFS_MEM_NODE)0)->data)
#else
#define BLOCK_DATA_OFFSET               (gctUINT) (gctUINT64) &(((gcsMEM_BLOCK)0)->data)
#define FS_MEM_NODE_DATA_OFFSET         (gctUINT) (gctUINT64) &(((gcsMEM_FS_MEM_NODE)0)->data)
#define VS_MEM_NODE_DATA_OFFSET         (gctUINT) (gctUINT64) &(((gcsMEM_VS_MEM_NODE)0)->next)
#define AFS_MEM_NODE_DATA_OFFSET        (gctUINT) (gctUINT64) &(((gcsMEM_AFS_MEM_NODE)0)->data)
#endif
#endif

#define _AdjustSize(Size) \
{ \
    gctUINT _adjustIndex; \
\
    for (_adjustIndex = 0; _adjustIndex < _BLOCKSIZE_TABLE_SIZE; _adjustIndex++) \
    { \
        if (*Size <= _blockSize[_adjustIndex]) \
        { \
            *Size = _blockSize[_adjustIndex]; \
            break; \
        } \
    } \
    gcmASSERT(_adjustIndex < _BLOCKSIZE_TABLE_SIZE); \
}

gceSTATUS
gcfMEM_InitFSMemPool(
    IN gcsMEM_FS_MEM_POOL * MemPool,
    IN gcoOS                OS,
    IN gctUINT              NodeCount,
    IN gctUINT              NodeSize
    )
{
#if USE_LOCAL_MEMORY_POOL
    gceSTATUS               status = gcvSTATUS_OK;
    gcsMEM_FS_MEM_POOL      memPool;
    gctUINT                 size;
    gctPOINTER pointer = gcvNULL;

    /* Allocation the memPool. */
    size = gcmSIZEOF(struct _gcsMEM_FS_MEM_POOL);
    gcmONERROR(gcoOS_Allocate(OS, size, &pointer));
    memPool = pointer;

    if (NodeSize < FS_MEM_NODE_DATA_OFFSET)
    {
        /* Prevent list of char or short. */
        gcmASSERT(NodeSize >= FS_MEM_NODE_DATA_OFFSET);
        NodeSize = FS_MEM_NODE_DATA_OFFSET;
    }

    /* Adjust nodeCount to have a good blockSize to reduce systme fragmentatin. */
    if (NodeCount == 0)
    {
        gcmASSERT(NodeCount > 0);
        NodeCount = 1;
    }
    size = NodeCount * NodeSize + FS_MEM_NODE_DATA_OFFSET;
    _AdjustSize(&size);
    NodeCount = (size - FS_MEM_NODE_DATA_OFFSET) / NodeSize;

    memPool->blockList  = gcvNULL;
    memPool->freeList   = gcvNULL;
    memPool->nodeCount  = NodeCount;
    memPool->nodeSize   = NodeSize;
    memPool->blockSize  = size;

    *MemPool = memPool;
#else
    *MemPool = OS;
#endif

    return gcvSTATUS_OK;

#if USE_LOCAL_MEMORY_POOL
OnError:
    return status;
#endif
}

gceSTATUS
gcfMEM_FreeFSMemPool(
    IN gcsMEM_FS_MEM_POOL * MemPool
    )
{
#if USE_LOCAL_MEMORY_POOL
    gcsMEM_FS_MEM_POOL      memPool = *MemPool;

    while (memPool->blockList != gcvNULL)
    {
        gcsMEM_BLOCK block = memPool->blockList;
        memPool->blockList = block->next;
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, block));
    }

    /* Free the MemPoll. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, memPool));
    *MemPool = gcvNULL;
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfMEM_FSMemPoolGetANode(
    IN gcsMEM_FS_MEM_POOL   MemPool,
    OUT gctPOINTER *        Node
    )
{
#if USE_LOCAL_MEMORY_POOL
    gceSTATUS               status = gcvSTATUS_OK;

    if (MemPool->freeList == gcvNULL)
    {
        gcsMEM_BLOCK        block;
        gcsMEM_FS_MEM_NODE  node;
        gctUINT8_PTR        data;
        gctUINT             size = MemPool->nodeSize;
        gctUINT             i;
        gctPOINTER pointer = gcvNULL;

        /* Allocation next block and put them in free list. */
        gcmONERROR(gcoOS_Allocate(gcvNULL, MemPool->blockSize, &pointer));
        block = pointer;

        block->next = MemPool->blockList;
        MemPool->blockList = block;

        /* Create linked list. */
        node = MemPool->freeList = (gcsMEM_FS_MEM_NODE) ((gctUINT8_PTR) block + BLOCK_DATA_OFFSET);
        data = (gctUINT8_PTR) node + size;
        for (i = MemPool->nodeCount - 1; i > 0 ; i--, data += size)
        {
            node->next = (gcsMEM_FS_MEM_NODE) data;
            node = (gcsMEM_FS_MEM_NODE) data;
        }
        node->next = gcvNULL;
    }

    *Node = (gctPOINTER) MemPool->freeList;
    MemPool->freeList = MemPool->freeList->next;
#endif

    return gcvSTATUS_OK;

#if USE_LOCAL_MEMORY_POOL
OnError:
    return status;
#endif
}

gceSTATUS
gcfMEM_FSMemPoolFreeANode(
    IN gcsMEM_FS_MEM_POOL   MemPool,
    IN gctPOINTER           Node
    )
{
#if USE_LOCAL_MEMORY_POOL
    gcsMEM_FS_MEM_NODE      node = (gcsMEM_FS_MEM_NODE) Node;

    node->next = MemPool->freeList;
    MemPool->freeList = node;
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfMEM_FSMemPoolFreeAList(
    IN gcsMEM_FS_MEM_POOL   MemPool,
    IN gctPOINTER           FirstNode,
    IN gctPOINTER           LastNode
    )
{
    gceSTATUS status = gcvSTATUS_OK;
#if USE_LOCAL_MEMORY_POOL
    gcsMEM_FS_MEM_NODE      firstNode = (gcsMEM_FS_MEM_NODE) FirstNode;
    gcsMEM_FS_MEM_NODE      lastNode  = (gcsMEM_FS_MEM_NODE) LastNode;

    if ((FirstNode != gcvNULL) && (LastNode != gcvNULL))
    {
        lastNode->next = MemPool->freeList;
        MemPool->freeList = firstNode;
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
    }
#endif

    return status;
}

gceSTATUS
gcfMEM_InitVSMemPool(
    IN gcsMEM_VS_MEM_POOL * MemPool,
    IN gcoOS                OS,
    IN gctUINT              BlockSize,
    IN gctBOOL              RecycleFreeNode
    )
{
#if USE_LOCAL_MEMORY_POOL
    gceSTATUS               status = gcvSTATUS_OK;
    gcsMEM_VS_MEM_POOL      memPool;
    gctUINT                 size, i;
    gctPOINTER pointer = gcvNULL;

    /* Allocation the memPool. */
    size = gcmSIZEOF(struct _gcsMEM_VS_MEM_POOL);
    gcmONERROR(gcoOS_Allocate(OS, size, &pointer));
    memPool = pointer;

    /* Adjust nodeCount to have a good blockSize to reduce systme fragmentatin. */
    size = BlockSize + FS_MEM_NODE_DATA_OFFSET;
    _AdjustSize(&size);

    memPool->blockList       = gcvNULL;
    memPool->blockSize       = size;
    memPool->freeSize        = 0;
    memPool->freeData        = gcvNULL;
    memPool->recycleFreeNode = RecycleFreeNode;

    for (i = 0; i <= _VS_FREE_LIST_ARRAY_SIZE; i++)
        memPool->freeListArray[i] = gcvNULL;

    *MemPool = memPool;
#else
    *MemPool = OS;
#endif

    return gcvSTATUS_OK;

#if USE_LOCAL_MEMORY_POOL
OnError:
    return status;
#endif
}

gceSTATUS
gcfMEM_FreeVSMemPool(
    IN gcsMEM_VS_MEM_POOL * MemPool
    )
{
#if USE_LOCAL_MEMORY_POOL
    gcsMEM_VS_MEM_POOL      memPool = *MemPool;

    while (memPool->blockList != gcvNULL)
    {
        gcsMEM_BLOCK block = memPool->blockList;
        memPool->blockList = block->next;
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, block));
    }

    /* Free the MemPoll. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, memPool));
    *MemPool = gcvNULL;
#endif

    return gcvSTATUS_OK;
}

#define _AdjustAlignment(MemPool, Alignment) \
{ \
    gctUINT _adjustMask = Alignment - 1; \
\
    if (MemPool->recycleFreeNode) \
    { \
        gctUINT misAlignment = gcmPTR2INT32(MemPool->freeData + VS_MEM_NODE_DATA_OFFSET) & _adjustMask; \
        if (misAlignment > 0) \
        { \
            gctUINT adjustment = Alignment - misAlignment; \
            MemPool->freeData += adjustment; \
            MemPool->freeSize -= adjustment; \
        } \
    } \
    else \
    { \
        gctUINT misAlignment = gcmPTR2INT32(MemPool->freeData) & _adjustMask; \
        if (misAlignment > 0) \
        { \
            gctUINT adjustment = Alignment - misAlignment; \
            MemPool->freeData += adjustment; \
            MemPool->freeSize -= adjustment; \
        } \
    } \
}

gceSTATUS
gcfMEM_VSMemPoolGetANode(
    IN gcsMEM_VS_MEM_POOL   MemPool,
    IN gctUINT              Size,
    IN gctUINT              Alignment,
    OUT gctPOINTER *        Node
    )
{
#if USE_LOCAL_MEMORY_POOL
    gceSTATUS               status = gcvSTATUS_OK;
    gcsMEM_VS_MEM_NODE      list = gcvNULL;
    gctUINT                 mask = Alignment - 1;
    gctUINT                 size;
    gctUINT                 sizeIndex;

    gcmASSERT(Alignment == 1 || Alignment == 2 || Alignment == 4 || Alignment == 8 ||
            Alignment == 16 || Alignment == 32 || Alignment == 64);

    if (Size == 0)
    {
        gcmASSERT(Size > 0);
        *Node = gcvNULL;
        return gcvSTATUS_OK;
    }

    if (MemPool->recycleFreeNode)
    {
        sizeIndex = (Size + VS_MEM_NODE_DATA_OFFSET - 1) / _WORD_SIZE;
        size = (sizeIndex + 1) * _WORD_SIZE;
        if (sizeIndex < _VS_FREE_LIST_ARRAY_SIZE)
        {
            gcsMEM_VS_MEM_NODE plist = gcvNULL;

            for (list = MemPool->freeListArray[sizeIndex]; list; list = list->next)
            {
                /* Check alignment. */
                if (((gcmPTR2INT32((gctUINT8_PTR) list + VS_MEM_NODE_DATA_OFFSET)) & mask) == 0) break;
                plist = list;
            }
            if (list)
            {
                /* Unlink list from freeList. */
                if (plist)
                {
                    plist->next = list->next;
                }
                else
                {
                    MemPool->freeListArray[sizeIndex] = list->next;
                }
            }
        }
        else
        {
            gcsMEM_VS_MEM_NODE flist = MemPool->freeListArray[_VS_FREE_LIST_ARRAY_SIZE];
            if (flist && flist->sizeIndex < sizeIndex)
            {
                gcsMEM_VS_MEM_NODE pflist = gcvNULL;
                gcsMEM_VS_MEM_NODE plist = gcvNULL;

                for (; flist->next; flist = flist->next)
                {
                    /* Check alignment. */
                    if (((gcmPTR2INT32((gctUINT8_PTR) flist + VS_MEM_NODE_DATA_OFFSET)) & mask) == 0)
                    {
                        list = flist;
                        plist = pflist;
                    }
                    /* Check size. */
                    if (flist->next->sizeIndex < sizeIndex) break;
                    pflist = flist;
                }
                if (list)
                {
                    gcmASSERT(list && list->sizeIndex >= sizeIndex);

                    /* Unlink list from freeList. */
                    if (plist)
                    {
                        gcmASSERT(plist->next == list);
                        plist->next = list->next;
                    }
                    else
                    {
                        gcmASSERT(list == MemPool->freeListArray[_VS_FREE_LIST_ARRAY_SIZE]);
                        MemPool->freeListArray[_VS_FREE_LIST_ARRAY_SIZE] = list->next;
                    }
                }
            }
        }
    }
    else
    {
        sizeIndex = 0;
        size = Size;
    }

    if (list == gcvNULL)
    {
        /* Adjust alignment. */
        _AdjustAlignment(MemPool, Alignment);

        /* Check free size in current block is large enough. */
        if (MemPool->freeSize < size)
        {
            gcsMEM_BLOCK block;
            gctPOINTER pointer = gcvNULL;

            if (MemPool->recycleFreeNode)
            {
                if (MemPool->freeSize >= VS_MEM_NODE_DATA_OFFSET + _WORD_SIZE)
                {
                    list = (gcsMEM_VS_MEM_NODE) MemPool->freeData;
                    list->sizeIndex = (MemPool->freeSize - VS_MEM_NODE_DATA_OFFSET - 1) / _WORD_SIZE;
                    gcfMEM_VSMemPoolFreeANode(MemPool, (gctPOINTER) ((gctUINT8_PTR) list + VS_MEM_NODE_DATA_OFFSET));
                }
            }

            /* Allocate a new block. */
            if (size <= MemPool->blockSize + Alignment)
            {
                /* Allocate a new block with default block size. */
                gcmONERROR(gcoOS_Allocate(gcvNULL, MemPool->blockSize, &pointer));
                block = pointer;
                list = (gcsMEM_VS_MEM_NODE) ((gctUINT8_PTR) block + BLOCK_DATA_OFFSET);
                MemPool->freeData = (gctUINT8_PTR) list + size;
                MemPool->freeSize = MemPool->blockSize - BLOCK_DATA_OFFSET - size;
            }
            else
            {
                /* The size is too big for default block. */
                gcmONERROR(gcoOS_Allocate(gcvNULL, size + BLOCK_DATA_OFFSET + Alignment, &pointer));
                block = pointer;
                list = (gcsMEM_VS_MEM_NODE) ((gctUINT8_PTR) block + BLOCK_DATA_OFFSET);
                MemPool->freeData = gcvNULL;
                MemPool->freeSize = 0;
            }

            /* Add block to block list. */
            block->next = MemPool->blockList;
            MemPool->blockList = block;
        }

        /* Adjust alignment. */
        _AdjustAlignment(MemPool, Alignment);

        gcmASSERT(size <= MemPool->freeSize);

        /* Allocate a list from block. */
        list = (gcsMEM_VS_MEM_NODE) MemPool->freeData;
        MemPool->freeData += size;
        MemPool->freeSize -= size;

        if (list)
            list->sizeIndex = sizeIndex;
    }

    if (MemPool->recycleFreeNode)
    {
        *Node = (gctPOINTER) ((gctUINT8_PTR) list + VS_MEM_NODE_DATA_OFFSET);
    }
    else
    {
        *Node = (gctPOINTER) list;
    }

    gcmASSERT((gcmPTR2INT32(Node) & (Alignment - 1)) == 0);
#endif

    return gcvSTATUS_OK;

#if USE_LOCAL_MEMORY_POOL
OnError:
    return status;
#endif
}

gceSTATUS
gcfMEM_VSMemPoolFreeANode(
    IN gcsMEM_VS_MEM_POOL   MemPool,
    IN gctPOINTER           Node
    )
{
#if USE_LOCAL_MEMORY_POOL
    if (MemPool->recycleFreeNode)
    {
        gcsMEM_VS_MEM_NODE  list = (gcsMEM_VS_MEM_NODE) ((gctUINT8_PTR) Node - VS_MEM_NODE_DATA_OFFSET);
        gctUINT             sizeIndex = list->sizeIndex;

        if (sizeIndex < _VS_FREE_LIST_ARRAY_SIZE)
        {
            list->next = MemPool->freeListArray[sizeIndex];
            MemPool->freeListArray[sizeIndex] = list;
        }
        else
        {
            gcsMEM_VS_MEM_NODE *    freeList = &MemPool->freeListArray[_VS_FREE_LIST_ARRAY_SIZE];
            gcsMEM_VS_MEM_NODE      flist, plist;

            /* Insert list to freeList in descending order in terms of size. */
            if (*freeList == gcvNULL)
            {
                list->next = gcvNULL;
                *freeList = list;
                return gcvSTATUS_OK;
            }

            plist = gcvNULL;
            for (flist = *freeList; flist; flist = flist->next)
            {
                if (flist->sizeIndex <= list->sizeIndex) break;
                plist = flist;
            }

            list->next = flist;
            if (plist)
            {
                plist->next = list;
            }
            else
            {
                *freeList = list;
            }
        }
    }
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfMEM_InitAFSMemPool(
    IN gcsMEM_AFS_MEM_POOL *MemPool,
    IN gcoOS                OS,
    IN gctUINT              NodeCount,
    IN gctUINT              NodeSize
    )
{
#if USE_LOCAL_MEMORY_POOL
    gceSTATUS               status = gcvSTATUS_OK;
    gcsMEM_AFS_MEM_POOL     memPool;
    gctUINT                 size;
    gctPOINTER pointer = gcvNULL;

    /* Allocation the memPool. */
    size = gcmSIZEOF(struct _gcsMEM_AFS_MEM_POOL);
    gcmONERROR(gcoOS_Allocate(OS, size, &pointer));
    memPool = pointer;

    /* Adjust nodeCount to have a good blockSize to reduce systme fragmentatin. */
    size = NodeCount * NodeSize + AFS_MEM_NODE_DATA_OFFSET + FS_MEM_NODE_DATA_OFFSET;
    _AdjustSize(&size);
    NodeCount = (size - AFS_MEM_NODE_DATA_OFFSET - FS_MEM_NODE_DATA_OFFSET) / NodeSize;
    gcmASSERT(size >= NodeCount * NodeSize + AFS_MEM_NODE_DATA_OFFSET + FS_MEM_NODE_DATA_OFFSET);

    memPool->blockList  = gcvNULL;
    memPool->freeList   = gcvNULL;
    memPool->nodeCount  = NodeCount;
    memPool->nodeSize   = NodeSize;
    memPool->blockSize  = size;
    memPool->freeSize   = 0;
    memPool->freeData   = gcvNULL;

    *MemPool = memPool;
#else
    *MemPool = OS;
#endif

    return gcvSTATUS_OK;

#if USE_LOCAL_MEMORY_POOL
OnError:
    return status;
#endif
}

gceSTATUS
gcfMEM_FreeAFSMemPool(
    IN gcsMEM_AFS_MEM_POOL *MemPool
    )
{
#if USE_LOCAL_MEMORY_POOL
    gcsMEM_AFS_MEM_POOL     memPool = *MemPool;

#if NEW_MEMPOOL
    while (memPool->blockList != gcvNULL)
    {
        gcsMEM_BLOCK block = memPool->blockList;
        memPool->blockList = block->next;
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, block));
    }
#else
    while (memPool->blockList != gcvNULL)
    {
        gcsMEM_AFS_MEM_NODE block = memPool->blockList;

        memPool->blockList = block->next;
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, block));
    }

    while (memPool->freeList != gcvNULL)
    {
        gcsMEM_AFS_MEM_NODE block = memPool->freeList;

        memPool->freeList = block->next;
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, block));
    }
#endif

    /* Free the MemPoll. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, memPool));
    *MemPool = gcvNULL;
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfMEM_AFSMemPoolGetANode(
    IN gcsMEM_AFS_MEM_POOL  MemPool,
    IN gctUINT              Count,
    OUT gctPOINTER *        Node
    )
{
#if USE_LOCAL_MEMORY_POOL
    gceSTATUS               status = gcvSTATUS_OK;
    gcsMEM_AFS_MEM_NODE     list = MemPool->freeList;

    if (list == gcvNULL || list->nodeCount < Count)
    {
#if NEW_MEMPOOL
        gctUINT size = Count * MemPool->nodeSize + AFS_MEM_NODE_DATA_OFFSET;
        gcsMEM_BLOCK block;

        /* Pad size to word size. */
        if ((size & (_WORD_SIZE - 1)) != 0)
        {
            size += _WORD_SIZE - (size & (_WORD_SIZE - 1));
        }

        if (size <= MemPool->freeSize)
        {
            list = (gcsMEM_AFS_MEM_NODE) MemPool->freeData;
            MemPool->freeData += size;
            MemPool->freeSize -= size;
        }
        else
        {
            gctPOINTER pointer = gcvNULL;

            if (MemPool->freeSize >= AFS_MEM_NODE_DATA_OFFSET + MemPool->nodeSize)
            {
                list = (gcsMEM_AFS_MEM_NODE) MemPool->freeData;
                list->nodeCount = (MemPool->freeSize - AFS_MEM_NODE_DATA_OFFSET) / MemPool->nodeSize;
                gcmASSERT(list->nodeCount > 0);
#if gcmIS_DEBUG(gcdDEBUG_CODE)
                list->isFreed = gcvFALSE;
#endif
                gcfMEM_AFSMemPoolFreeANode(MemPool, (gctPOINTER) ((gctUINT8_PTR) list + AFS_MEM_NODE_DATA_OFFSET));
            }

            /* Allocate a new block. */
            if (Count <= MemPool->nodeCount)
            {
                /* Allocate a new block with default block size. */
                gcmONERROR(gcoOS_Allocate(gcvNULL, MemPool->blockSize, &pointer));
                block = pointer;
                list = (gcsMEM_AFS_MEM_NODE) ((gctUINT8_PTR) block + BLOCK_DATA_OFFSET);
                MemPool->freeData = (gctUINT8_PTR) list + size;
                MemPool->freeSize = MemPool->blockSize - BLOCK_DATA_OFFSET - size;
            }
            else
            {
                /* The array is too big for default block. */
                gcmONERROR(gcoOS_Allocate(gcvNULL, size + BLOCK_DATA_OFFSET, &pointer));
                block = pointer;
                list = (gcsMEM_AFS_MEM_NODE) ((gctUINT8_PTR) block + BLOCK_DATA_OFFSET);
                MemPool->freeData = gcvNULL;
                MemPool->freeSize = 0;
            }

            /* Add block to block list. */
            block->next = MemPool->blockList;
            MemPool->blockList = block;
        }
#else
        /* Allocate a new array node. */
        gcmONERROR(gcoOS_Allocate(MemPool->os,
                        Count * MemPool->nodeSize + AFS_MEM_NODE_DATA_OFFSET,
                        (gctPOINTER *) &list));

        /* Add list to block list. */
        list->prev = gcvNULL;
        list->next = MemPool->blockList;
        if (MemPool->blockList) MemPool->blockList->prev = list;
        MemPool->blockList = list;
#endif

        list->nodeCount = Count;
    }
    else
    {
        for (; list->next; list = list->next)
        {
            if (list->next->nodeCount < Count) break;
        }
        gcmASSERT(list && list->nodeCount >= Count);

        /* Unlink list from freeList. */
        if (list->prev != gcvNULL)
        {
            list->prev->next = list->next;
        }
        else
        {
            gcmASSERT(list == MemPool->freeList);
            MemPool->freeList = list->next;
        }
        if (list->next != gcvNULL)
        {
            list->next->prev = list->prev;
        }

#if !NEW_MEMPOOL
        /* Add list to block list. */
        list->prev = gcvNULL;
        list->next = MemPool->blockList;
        if (MemPool->blockList) MemPool->blockList->prev = list;
        MemPool->blockList = list;
#endif
    }

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    list->isFreed = gcvFALSE;
#endif
    *Node = (gctPOINTER) ((gctUINT8_PTR) list + AFS_MEM_NODE_DATA_OFFSET);
#endif

    return gcvSTATUS_OK;

#if USE_LOCAL_MEMORY_POOL
OnError:
    return status;
#endif
}

gceSTATUS
gcfMEM_AFSMemPoolFreeANode(
    IN gcsMEM_AFS_MEM_POOL  MemPool,
    IN gctPOINTER           Node
    )
{
#if USE_LOCAL_MEMORY_POOL
    gcsMEM_AFS_MEM_NODE     list = (gcsMEM_AFS_MEM_NODE) ((gctUINT8_PTR) Node - AFS_MEM_NODE_DATA_OFFSET);
    gcsMEM_AFS_MEM_NODE     flist, plist;

#if gcmIS_DEBUG(gcdDEBUG_CODE)
    if (list->isFreed == gcvTRUE)
    {
        gcmASSERT(list->isFreed == gcvFALSE);
        return gcvSTATUS_OK;
    }
    list->isFreed = gcvTRUE;
#endif

#if !NEW_MEMPOOL
    /* Unlink list from blockList. */
    if (list->prev != gcvNULL)
    {
        list->prev->next = list->next;
    }
    else
    {
        gcmASSERT(list == MemPool->blockList);
        MemPool->blockList = list->next;
    }
    if (list->next != gcvNULL)
    {
        list->next->prev = list->prev;
    }
#endif

    /* Insert list to freeList in descending order in terms of count. */
    if (MemPool->freeList == gcvNULL)
    {
        list->prev = list->next = gcvNULL;
        MemPool->freeList = list;
        return gcvSTATUS_OK;
    }

    plist = gcvNULL;
    for (flist = MemPool->freeList; flist; flist = flist->next)
    {
        if (flist->nodeCount <= list->nodeCount) break;
        plist = flist;
    }

    if (plist)
    {
        list->next = flist;
        list->prev = plist;
        if (flist)
        {
            flist->prev = list;
        }
        plist->next = list;
    }
    else
    {
        gcmASSERT(flist == MemPool->freeList);
        list->prev = gcvNULL;
        list->next = flist;
        flist->prev = list;
        MemPool->freeList = list;
    }
#endif

    return gcvSTATUS_OK;
}
#endif /*(gcdENABLE_3D || gcdENABLE_VG)*/

