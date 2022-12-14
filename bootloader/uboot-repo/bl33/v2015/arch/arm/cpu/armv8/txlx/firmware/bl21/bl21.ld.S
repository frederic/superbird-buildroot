/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
* *
This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* *
This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
* *
You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
* *
Description:
*/

#ifdef AARCH32
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
#else
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
#endif

ENTRY(bl21_entrypoint)

SECTIONS
{
    . = 0xFFFCB400;
    ASSERT(. == ALIGN(1024),
           "BL21 base address is not aligned.")

    ro . : {
        __RO_START__ = .;
        *bl21_entrypoint.o(.text*)
        *(.text*)
        *(.rodata*)
        *(.vectors)
        __RO_END_UNALIGNED__ = .;
        /*
         * Memory page(s) mapped to this section will be marked as
         * read-only, executable.  No RW data from the next section must
         * creep in.  Ensure the rest of the current memory page is unused.
         */
        __RO_END__ = .;
    }

    .data . : {
        __DATA_START__ = .;
        *(.data*)
        __DATA_END__ = .;
    }

    /*
     * The .bss section gets initialised to 0 at runtime.
     * Its base address must be 16-byte aligned.
     */
    .bss : ALIGN(16) {
        __BSS_START__ = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        __BSS_END__ = .;
    }

    __BSS_SIZE__ = SIZEOF(.bss);

    ASSERT(. <= (0xFFFCB400 + 0xC00), "bl21 image has exceeded its limit.")
}
