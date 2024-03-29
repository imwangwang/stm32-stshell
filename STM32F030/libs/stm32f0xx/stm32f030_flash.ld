/* fixed */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)        : ORIGIN = 0x08000000, LENGTH = 16K
  RAM (xrw)         : ORIGIN = 0x20000000, LENGTH = 4K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

MIN_HEAP_SIZE = 0;
MIN_STACK_SIZE = 256;

SECTIONS
{
  /* Interrupt vector table */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    /* Code */
    *(.text)
    *(.text*)
    /* Constants */
    *(.rodata)
    *(.rodata*)
    /* ARM->Thumb and Thumb->ARM glue code */
    *(.glue_7)
    *(.glue_7t)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH
    
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array*))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  _sidata = .;

  /* Initialized data */
  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;   /* create a global symbol at data start */
    *(.data)
    *(.data*)

    . = ALIGN(4);
    _edata = .;   /* define a global symbol at data end */
  } >RAM

  /* Uninitialized data */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;    /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;    /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  PROVIDE(end = _ebss);
  PROVIDE(_end = _ebss);

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(4);
    . = . + MIN_HEAP_SIZE;
    . = . + MIN_STACK_SIZE;
    . = ALIGN(4);
  } >RAM

  /DISCARD/ :
  {
    libc.a(*)
    libm.a(*)
    libgcc.a(*)
    *(.note.gnu.build-id) 
    *(.comment) 
    *(.ARM.attributes) 
  }
}

