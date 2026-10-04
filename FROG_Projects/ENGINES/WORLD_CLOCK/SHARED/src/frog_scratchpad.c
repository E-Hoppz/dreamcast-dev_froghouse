#include "frog_scratchpad.h"
#include <arch/irq.h> // For safe interrupt disabling

#define SH4_CCR_ADDR 0xff00001c

static uint32_t scratchpad_offset = 0;

void frog_scratchpad_init(void) {
    // 1. Disable interrupts to prevent context switches during cache split
    int old_irq = irq_disable();

    volatile uint32_t *ccr = (volatile uint32_t *)SH4_CCR_ADDR;
    uint32_t val = *ccr;

    // Set the O_RAM bit (bit 10) in the Cache Control Register.
    // Splits the 8KB L1 Data Cache into 4KB cache and 4KB Scratchpad RAM (0x7c000000)
    val |= (1 << 10); 

    // Write back to hardware register
    *ccr = val;

    // Execute 8 NOPs to allow the SH-4 pipeline and cache controller to stabilize
    __asm__ __volatile__(
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
        "nop\n\t"
    );

    // Re-enable interrupts
    irq_restore(old_irq);
}

void* frog_scratchpad_alloc(size_t size) {
    // Align allocations to 4-byte boundaries to match bus widths
    size = (size + 3) & ~3;
    
    if (scratchpad_offset + size > 4096) {
        return NULL; // Out of 4KB Scratchpad RAM
    }
    
    void *ptr = (void *)(0x7c000000 + scratchpad_offset);
    scratchpad_offset += size;
    return ptr;
}

void frog_scratchpad_reset(void) {
    scratchpad_offset = 0;
}
