/****************************************************************************
Copyright (C) Cambridge Silicon Radio Ltd. 2004-2010
Part of HeadsetSDK-Stereo R110.0

FILE NAME
    a2dp_data_block.c

DESCRIPTION

NOTES

*/



/****************************************************************************
    Header files
*/

#include "a2dp_data_block.h"
#include "a2dp_private.h"
#include <string.h>    /* for memset */
#include <print.h>
#include <stdlib.h>


/* Initial version will have to use 2 slots, one for the A2DP task block (which must remain fixed) and the other for the data blocks */
uint8 *blockAdd (uint8 device_id, data_block_id block_id, uint8 element_count, uint8 element_size)
{
    data_block_info *data_block = &a2dp->sep.data_blocks.block[device_id][block_id];
    
    PRINT(("blockAdd(devId=%u, blkId=%u, cnt=%u, sz=%u)",device_id, block_id, element_count, element_size));
    
    if (!data_block->offset)
    {
        uint16 block_size = element_size * element_count;
        uint16 offset = sizeof(A2DP) + a2dp->sep.data_blocks.size_blocks;  /* New block added at end of any existing ones */
        A2DP *new_a2dp = (A2DP *)realloc(a2dp, offset+block_size);
        if (new_a2dp)
        {
            a2dp = new_a2dp;
            memset((uint8*)((uint16)a2dp+offset), 0, block_size);  /* For debug purposes */
            data_block->offset = offset;
            data_block->block_size = block_size;
            data_block->element_size = element_size;
            data_block->current_element = 0;
            a2dp->sep.data_blocks.size_blocks += block_size;

            PRINT((" [@%X]  size_blocks=%u\n",(uint16)a2dp+offset, a2dp->sep.data_blocks.size_blocks));
            return (uint8 *)((uint16)a2dp+offset);
        }
    }

    /* Failed - block already exists  or  unable to allocate memory */
    PRINT(("[NULL]\n"));
    return 0;
}


void blockRemove (uint8 device_id, data_block_id block_id)
{
    data_block_info *data_block = &a2dp->sep.data_blocks.block[device_id][block_id];
    uint16 offset = data_block->offset;
    
    PRINT(("blockRemove(devId=%u, blkId=%u)",device_id, block_id));
    
    if ( offset )
    {
        A2DP *new_a2dp;
        uint16 block_size = data_block->block_size;
        data_block_info *block = &a2dp->sep.data_blocks.block[0][0];
        
        /* Reduce offsets of all blocks positioned above the block being removed */        
        do
        {
            PRINT((" [%X]",(uint16)block));
            
            if ( offset < block->offset )
            {
                block->offset -= block_size;
            }
        }
        while (++block <= &a2dp->sep.data_blocks.block[A2DP_MAX_REMOTE_DEVICES-1][max_data_blocks-1]);
        
        /* Zero info parameters of block being removed */
        memset( data_block, 0, sizeof( data_block_info ) );
        
        /* Reduce overall size of all blocks */
        a2dp->sep.data_blocks.size_blocks -= block_size;
        
        PRINT(("  size_blocks=%u\n",a2dp->sep.data_blocks.size_blocks));
            
        /* Shift blocks above removed block down by the appropriate amount.  For debug purposes, fill the now unused area at top of memory area */
        memmove((uint8*)((uint16)a2dp+offset), (uint8*)((uint16)a2dp+offset+block_size), a2dp->sep.data_blocks.size_blocks-offset+sizeof(A2DP));
        memset((uint8*)((uint16)a2dp+a2dp->sep.data_blocks.size_blocks+sizeof(A2DP)), 0xFF, block_size);  /* For debug purposes */
        
        if ( (new_a2dp = (A2DP *)realloc(a2dp, sizeof(A2DP)+a2dp->sep.data_blocks.size_blocks)) != NULL )
        {
            a2dp = new_a2dp;
        }
        /* No need to worry about a failed realloc, old one will still exist and be valid */
    }
}


uint8 *blockGetBase (uint8 device_id, data_block_id block_id)
{
    uint16 offset = a2dp->sep.data_blocks.block[device_id][block_id].offset;
    
    PRINT(("blockGetBase(devId=%u, blkId=%u)\n",device_id, block_id));
    
    return (offset)?(uint8*)((uint16)a2dp+offset):(uint8*)0;
}


uint8 *blockGetIndexed (uint8 device_id, data_block_id block_id, uint8 element)
{
    data_block_info *data_block = &a2dp->sep.data_blocks.block[device_id][block_id];
    uint16 offset = data_block->element_size * element;
    
    PRINT(("blockGetIndexed(devId=%u, blkId=%u, ele=%u)\n",device_id, block_id, element));
    
    offset += data_block->offset;
    return (offset)?(uint8*)((uint16)a2dp+offset):(uint8*)0;
}


uint8 *blockGetCurrent (uint8 device_id, data_block_id block_id)
{
    data_block_info *data_block = &a2dp->sep.data_blocks.block[device_id][block_id];
    uint16 offset = data_block->element_size * data_block->current_element;
    
    PRINT(("blockGetCurrent(devId=%u, blkId=%u)=%u\n",device_id, block_id, data_block->current_element));
    
    offset += data_block->offset;
    return (offset)?(uint8*)((uint16)a2dp+offset):(uint8*)0;
}


bool blockSetCurrent (uint8 device_id, data_block_id block_id, uint8 element)
{
    data_block_info *data_block = &a2dp->sep.data_blocks.block[device_id][block_id];
    
    PRINT(("blockSetCurrent(devId=%u, blkId=%u, ele=%u)\n",device_id, block_id, element));
    
    if ( element == DATA_BLOCK_INDEX_NEXT )
    {
        element = data_block->current_element + 1;
    }
    else if ( element == DATA_BLOCK_INDEX_PREVIOUS )
    {
        element = data_block->current_element - 1;
    }
    
    if ( (data_block->element_size * element) < data_block->block_size )
    {
        data_block->current_element = element;
        return TRUE;
    }

    return FALSE;
}


uint16 blockGetSize (uint8 device_id, data_block_id block_id)
{
    PRINT(("blockGetSize(devId=%u, blkId=%u)=%u   size_blocks=%u\n",device_id, block_id, a2dp->sep.data_blocks.block[device_id][block_id].block_size, a2dp->sep.data_blocks.size_blocks));
    return a2dp->sep.data_blocks.block[device_id][block_id].block_size;
}


