#include <linux/string.h>
#include <linux/types.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

// 128-bit primitives

void write_buffer_m128(uint8_t *addr, uint8_t *value)
{
    asm volatile (
        "movdqu (%1), %%xmm0\n"
        "movdqa %%xmm0, (%0)\n"
        :
        : "r" (addr), "r" (value)
        : "memory"
    );
}

void read_buffer_m128(uint8_t *addr, uint8_t *value)
{
    asm volatile (
        "movdqa (%0), %%xmm0\n"
        "movdqu %%xmm0, (%1)\n"
        :
        : "r" (addr), "r" (value)
        : "memory"
    );
}

// 256-bit primitives

void write_buffer_m256(uint8_t *addr, uint8_t *value)
{
    asm volatile (
        "vmovdqu (%1), %%ymm0\n"
        "vmovdqa %%ymm0, (%0)\n"
        :
        : "r" (addr), "r" (value)
        : "memory"
    );
}

void read_buffer_m256(void *addr, uint8_t *value)
{
    asm volatile (
        "vmovdqa (%0), %%ymm0\n"
        "vmovdqu %%ymm0, (%1)\n"
        :
        : "r" (addr), "r" (value)
        : "memory"
    );
}

// fxsave primitives

__attribute__((aligned(16)))
struct fxsave_area {
    uint16_t fcw;
    uint16_t fsw;
    uint8_t  ftw;
    uint8_t  reserved1;
    uint16_t fop;
    uint64_t fpu_ip;
    uint64_t fpu_dp;
    uint32_t mxcsr;
    uint32_t mxcsr_mask;
    uint8_t  st_space[128];   // 8*16 bytes for ST0-ST7 // offset 256
    uint8_t  xmm_space[256];  // 16*16 bytes for XMM0-XMM15
    uint8_t  reserved2[96];
};

void asm_fxrstor(void *target)
{
    __asm__ volatile("fxrstorq (%0)" :: "r"(target) : );
}

void asm_fxsave(void *target)
{
    __asm__ volatile("fxsave (%0)" :: "r"(target) : );
}    __attribute__((aligned(32)))

void _write_buffer_fxsave(void *addr_ctx, void *buffer, long size)
{
    struct fxsave_area *fxsave_area = (struct fxsave_area *)addr_ctx;
    memcpy(fxsave_area->st_space, buffer, size);
}

void write_fxsave(void *ctx, void *buffer, long size)
{
    unsigned char *ctx_u8 = (unsigned char *)ctx;
    ctx_u8 -= 256; // addr of the context to pass to fxsave
    __attribute__((aligned((32))))
    struct fxsave_area tmp_ctx = {0}; // temporary buffer for fxsave
    asm_fxsave(&tmp_ctx); // setup flags in temporary context
    memcpy(&(tmp_ctx.st_space), buffer, size); // setup the internal registers
    asm_fxrstor(&tmp_ctx); // restore the context
    asm_fxsave((void *)ctx_u8); // save the context to trigger the write
}

//xsave primitives
//todo

// main wrapper function

void wrapper_main_logic(void (*main_logic)(void))
{
    __attribute__((aligned(32)))
    uint8_t context_save[sizeof(struct fxsave_area)];
    asm_fxsave(context_save);
    main_logic();
    asm_fxrstor(context_save);
}
