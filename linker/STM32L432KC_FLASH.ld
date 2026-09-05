/* STM32L432KC - 256 KB Flash, 48 KB SRAM1 */

ENTRY(Reset_Handler)

/* Initial stack pointer */
_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x0000;
_Min_Stack_Size = 0x0400;   /* 1 KB */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 48K
}

SECTIONS
{
    /* Interrupt vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);

        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Constructors */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    /* Address of initialized data in Flash */
    _sidata = LOADADDR(.data);

    /* Initialized data */
    .data :
    {
        . = ALIGN(4);

        _sdata = .;

        *(.data)
        *(.data*)

        . = ALIGN(4);
        _edata = .;

    } > RAM AT > FLASH

    /* Zero-initialized data */
    .bss :
    {
        . = ALIGN(4);

        _sbss = .;

        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;

    } > RAM

    /* Heap and Stack */
    ._user_heap_stack :
    {
        . = ALIGN(8);

        PROVIDE(end = .);
        PROVIDE(_end = .);

        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;

        . = ALIGN(8);

    } > RAM

    /* Remove unused standard library sections */
    /DISCARD/ :
    {
        *(.note*)
        *(.comment*)
    }
}