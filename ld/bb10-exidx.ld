/* Added to binutils' default armnto linker script, which has no rules for
   the ARM EHABI unwind tables: without them, the .ARM.exidx.* table of every
   inline or template function becomes an output section of its own, outside
   the PT_ARM_EXIDX segment the unwinder reads, and exceptions cannot pass
   through those functions. These are the generic ARM ELF script's rules. */
SECTIONS
{
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) }
  PROVIDE_HIDDEN (__exidx_start = .);
  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) }
  PROVIDE_HIDDEN (__exidx_end = .);
}
INSERT AFTER .rodata1;
