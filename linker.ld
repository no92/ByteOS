ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

phys = 0x100000;
offset = 0xFFFFFFFF80000000;
virt = offset + phys;

SECTIONS
{
        . = virt;

        .rodata : AT(ADDR(.rodata) - offset)
        {
                _rodata_begin_phys = . - offset;
                *(.multiboot_header)
                *(.rodata)
                *(.rodata*)
                . = ALIGN(0x1000);
                _rodata_end_phys = . - offset;
        }

        .text : AT(ADDR(.text) - offset)
        {
                _text_begin_phys = . - offset;
                *(.text)
                . = ALIGN(0x1000);
                _text_end_phys = . - offset;
        }

        .data : AT(ADDR(.data) - offset)
        {
                _data_begin_phys = . - offset;
                *(.data)
                . = ALIGN(0x1000);
                _data_end_phys = . - offset;
        }

        .bss : AT(ADDR(.bss) - offset)
        {
                _bss_begin_phys = . - offset;
                *(.bss)
                *(COMMON)
                . = ALIGN(0x1000);
                _bss_end_phys = . - offset;
        }

        /DISCARD/ :
        {
                *(.comment)
                *(.eh_frame)
        }

        _kernel_end_phys = . - offset;

}
