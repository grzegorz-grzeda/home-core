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
    . = ALIGN(4);
} > FLASH

.data :
{
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
} > RAM AT > FLASH

_sidata = LOADADDR(.data);

.bss :
{
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(8);
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
ASSERT(_heap_start <= _heap_end, "Static RAM and main stack exceed RAM")
