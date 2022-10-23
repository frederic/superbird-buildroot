//----------------------------------------------------------------------------
//   The confidential and proprietary information contained in this file may
//   only be used by a person authorised under and to the extent permitted
//   by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT [2018] ARM Limited or its affiliates.
//              ALL RIGHTS RESERVED
//
//   This entire notice must be reproduced on all copies of this file
//   and copies of this file may only be made by a person if such person is
//   permitted to do so under the terms of a subsisting license agreement
//   from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#if !defined( __ACAMERA_BIT_MASK_H__ )
#define __ACAMERA_BIT_MASK_H__


#define ACAMERA_BITMASK_CELL_TYPE uint32_t
#define ACAMERA_BITMASK_BITS_PER_CELL ( sizeof( ACAMERA_BITMASK_CELL_TYPE ) * 8 )
#define ACAMERA_BITMASK_CELL_COUNT( bits_count ) \
    ( ( bits_count + ACAMERA_BITMASK_BITS_PER_CELL - 1 ) / ACAMERA_BITMASK_BITS_PER_CELL )

static __inline uint8_t acamera_bitmask_get_bit( const ACAMERA_BITMASK_CELL_TYPE *p_bits, int idx )
{
    return ( p_bits[idx / ACAMERA_BITMASK_BITS_PER_CELL] >> ( idx % ACAMERA_BITMASK_BITS_PER_CELL ) ) & 1;
}

static __inline void acamera_bitmask_set_bit( ACAMERA_BITMASK_CELL_TYPE *p_bits, int idx )
{
    p_bits[idx / ACAMERA_BITMASK_BITS_PER_CELL] |= 1 << ( idx % ACAMERA_BITMASK_BITS_PER_CELL );
}

static __inline void acamera_bitmask_clear_bit( ACAMERA_BITMASK_CELL_TYPE *p_bits, int idx )
{
    p_bits[idx / ACAMERA_BITMASK_BITS_PER_CELL] &= ~( 1 << ( idx % ACAMERA_BITMASK_BITS_PER_CELL ) );
}

#endif /* __ACAMERA_BIT_MASK_H__ */
