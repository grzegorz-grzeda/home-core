ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

__stack_size = DEFINED(__stack_size) ? __stack_size : 4K;
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        __isr_vector_start = .;

        KEEP(*(.isr_vector.arch))
        KEEP(*(.isr_vector.soc))

        . = ALIGN(4);
        __isr_vector_end = .;
    } > FLASH

    .text :
    {
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
    } > FLASH

    .data :
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    . = ALIGN(8);
    _heap_start = .;

    .stack_dummy :
    {
        . = . + __stack_size;
    } > RAM
    . = ALIGN(8);

    _heap_end = ORIGIN(RAM) + LENGTH(RAM) - __stack_size;
}