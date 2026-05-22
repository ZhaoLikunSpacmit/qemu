/*
 * AME mmacc regression tests.
 *
 * Verifies `mmacc.w.b` operand orientation follows `A(ms1) * B_T(ms2)`.
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ame_insn.h"

#define CSR_XTLENB  0xcc1
#define CSR_XTRLENB 0xcc2

#define AME_STRIDE_TILE_LOAD_ENC(func4, size, td, rs1, rs2) \
    (((uint32_t)(func4) << 28) | ((uint32_t)0x1 << 26) | \
     (((uint32_t)(rs2) & 0x1fu) << 20) | (((uint32_t)(rs1) & 0x1fu) << 15) | \
     (((uint32_t)(size) & 0x3u) << 10) | (((uint32_t)(td) & 0x3u) << 7) | \
     AME_OP)

#define MLAE8(td, rs1, rs2) AME_STRIDE_TILE_LOAD_ENC(0x0, 0x0, td, rs1, rs2)

enum {
    AME_TILE_BYTES = 128,
    AME_ACC_WORDS = 4096 / sizeof(int32_t),
};

static inline unsigned long read_csr(unsigned csr)
{
    unsigned long value;

    __asm__ volatile ("csrr %0, %1" : "=r"(value) : "i"(csr));
    return value;
}

static inline void load_tile8(unsigned reg, const void *ptr)
{
    register const void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 0:
        __asm__ volatile (".4byte %0" :: "i"(MLME8(0, 10)), "r"(a0) : "memory");
        break;
    case 1:
        __asm__ volatile (".4byte %0" :: "i"(MLME8(1, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported tile load register");
        break;
    }
}

static inline void load_tile8_stride(unsigned reg, const void *ptr,
                                     ptrdiff_t stride)
{
    register const void *a0 __asm__("a0") = ptr;
    register ptrdiff_t a1 __asm__("a1") = stride;

    switch (reg) {
    case 0:
        __asm__ volatile (".4byte %0" :: "i"(MLAE8(0, 10, 11)),
                          "r"(a0), "r"(a1) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported tile stride load register");
        break;
    }
}

static inline void store_tile8(unsigned reg, void *ptr)
{
    register void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 0:
        __asm__ volatile (".4byte %0" :: "i"(MSME8(0, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported tile store register");
        break;
    }
}

static inline void load_tile16(unsigned reg, const void *ptr)
{
    register const void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 0:
        __asm__ volatile (".4byte %0" :: "i"(MLME16(0, 10)), "r"(a0) : "memory");
        break;
    case 1:
        __asm__ volatile (".4byte %0" :: "i"(MLME16(1, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported tile16 load register");
        break;
    }
}

static inline void load_acc32(unsigned reg, const void *ptr)
{
    register const void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 4:
        __asm__ volatile (".4byte %0" :: "i"(MLME32(4, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported acc load register");
        break;
    }
}

static inline void store_acc32(unsigned reg, void *ptr)
{
    register void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 4:
        __asm__ volatile (".4byte %0" :: "i"(MSME32(4, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported acc store register");
        break;
    }
}

static inline void load_acc16(unsigned reg, const void *ptr)
{
    register const void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 4:
        __asm__ volatile (".4byte %0" :: "i"(MLME16(4, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported acc16 load register");
        break;
    }
}

static inline void store_acc16(unsigned reg, void *ptr)
{
    register void *a0 __asm__("a0") = ptr;

    switch (reg) {
    case 4:
        __asm__ volatile (".4byte %0" :: "i"(MSME16(4, 10)), "r"(a0) : "memory");
        break;
    default:
        TEST_ASSERT(0, "unsupported acc16 store register");
        break;
    }
}

static void test_mmacc_operand_order(void)
{
    int8_t tile_a[AME_TILE_BYTES] = { 1, 2, 3, 4 };
    int8_t tile_bt[AME_TILE_BYTES] = { 5, 6, 7, 8 };
    int32_t acc_out[AME_ACC_WORDS];

    memset(acc_out, 0, sizeof(acc_out));

    AME_INSN(MZERO8R(0));
    AME_INSN(MSETTILEMI(2));
    AME_INSN(MSETTILENI(2));
    AME_INSN(MSETTILEKI(2));

    load_tile8(0, tile_a);
    load_tile8(1, tile_bt);

    AME_INSN(MMACC_W_B(0, 1, 0));

    store_acc32(4, acc_out);

    TEST_ASSERT(acc_out[0] == 17, "mmacc cell[0,0] matches A * B_T");
    TEST_ASSERT(acc_out[1] == 23, "mmacc cell[0,1] uses ms1 as A");
    TEST_ASSERT(acc_out[2] == 39, "mmacc cell[1,0] uses ms2 as B_T");
    TEST_ASSERT(acc_out[3] == 53, "mmacc cell[1,1] matches A * B_T");
}

static void test_strided_load_uses_physical_row_stride(void)
{
    const unsigned long row_bytes = read_csr(CSR_XTRLENB);
    uint8_t mem_src[4] = { 1, 2, 3, 4 };
    uint8_t mem_dst[AME_TILE_BYTES];

    memset(mem_dst, 0xff, sizeof(mem_dst));

    TEST_ASSERT(row_bytes > 2, "xtrlenb exposes physical row padding");

    AME_INSN(MZERO8R(0));
    AME_INSN(MSETTILEMI(2));
    AME_INSN(MSETTILENI(1));
    AME_INSN(MSETTILEKI(2));

    load_tile8_stride(0, mem_src, 2);
    store_tile8(0, mem_dst);

    TEST_ASSERT(mem_dst[0] == 1, "mlae8 keeps row0 col0");
    TEST_ASSERT(mem_dst[1] == 2, "mlae8 keeps row0 col1");
    TEST_ASSERT(mem_dst[row_bytes] == 3, "mlae8 uses physical row stride for row1 col0");
    TEST_ASSERT(mem_dst[row_bytes + 1] == 4, "mlae8 uses physical row stride for row1 col1");
    TEST_ASSERT(mem_dst[2] == 0, "mlae8 leaves row0 inactive bytes untouched");
    TEST_ASSERT(mem_dst[row_bytes + 2] == 0, "mlae8 leaves row1 inactive bytes untouched");
}

static void test_mmacc_whole_load_uses_physical_row_stride(void)
{
    const unsigned long row_bytes = read_csr(CSR_XTRLENB);
    int8_t tile_a[AME_TILE_BYTES];
    int8_t tile_bt[AME_TILE_BYTES];
    int32_t acc_out[AME_ACC_WORDS];

    memset(tile_a, 0, sizeof(tile_a));
    memset(tile_bt, 0, sizeof(tile_bt));
    memset(acc_out, 0, sizeof(acc_out));

    TEST_ASSERT(row_bytes > 2, "xtrlenb exposes multi-column physical rows");

    tile_a[0] = 1;
    tile_a[1] = 2;
    tile_a[row_bytes] = 3;
    tile_a[row_bytes + 1] = 4;

    tile_bt[0] = 5;
    tile_bt[1] = 6;
    tile_bt[row_bytes] = 7;
    tile_bt[row_bytes + 1] = 8;

    AME_INSN(MZERO8R(0));
    AME_INSN(MSETTILEMI(2));
    AME_INSN(MSETTILENI(2));
    AME_INSN(MSETTILEKI(2));

    load_tile8(0, tile_a);
    load_tile8(1, tile_bt);

    AME_INSN(MMACC_W_B(0, 1, 0));

    store_acc32(4, acc_out);

    TEST_ASSERT(acc_out[0] == 17, "whole-load mmacc cell[0,0] honors physical row stride");
    TEST_ASSERT(acc_out[1] == 23, "whole-load mmacc cell[0,1] honors physical row stride");
    TEST_ASSERT(acc_out[2] == 39, "whole-load mmacc cell[1,0] honors physical row stride");
    TEST_ASSERT(acc_out[3] == 53, "whole-load mmacc cell[1,1] honors physical row stride");
}

static void test_mmacc_clears_inactive_accumulator_region(void)
{
    const unsigned long tile_bytes = read_csr(CSR_XTLENB);
    const unsigned long row_bytes = read_csr(CSR_XTRLENB);
    const unsigned long rownum = tile_bytes / row_bytes;
    int8_t tile_a[AME_TILE_BYTES] = { 2 };
    int8_t tile_bt[AME_TILE_BYTES] = { 3 };
    int32_t acc_init[AME_ACC_WORDS];
    int32_t acc_out[AME_ACC_WORDS];
    unsigned long i;

    memset(acc_out, 0, sizeof(acc_out));
    for (i = 0; i < AME_ACC_WORDS; i++) {
        acc_init[i] = 0x55aa0000u + (int32_t)i;
    }

    TEST_ASSERT(rownum >= 2, "configuration has inactive accumulator region");

    AME_INSN(MZERO8R(0));
    AME_INSN(MSETTILEMI(1));
    AME_INSN(MSETTILENI(1));
    AME_INSN(MSETTILEKI(1));

    load_tile8(0, tile_a);
    load_tile8(1, tile_bt);
    load_acc32(4, acc_init);

    AME_INSN(MMACC_W_B(0, 1, 0));

    store_acc32(4, acc_out);

    TEST_ASSERT(acc_out[0] == acc_init[0] + 6,
                "mmacc keeps active accumulator cell and accumulates into it");

    for (i = 1; i < rownum; i++) {
        TEST_ASSERT(acc_out[i] == 0,
                    "mmacc clears inactive accumulator columns");
    }

    for (i = rownum; i < rownum * rownum; i++) {
        TEST_ASSERT(acc_out[i] == 0,
                    "mmacc clears inactive accumulator rows");
    }
}

static void test_mfmacc_s_h_clears_inactive_accumulator_region(void)
{
    const unsigned long tile_bytes = read_csr(CSR_XTLENB);
    const unsigned long row_bytes = read_csr(CSR_XTRLENB);
    const unsigned long rownum = tile_bytes / row_bytes;
    uint16_t tile_a[AME_TILE_BYTES / sizeof(uint16_t)] = { 0x3c00 };
    uint16_t tile_bt[AME_TILE_BYTES / sizeof(uint16_t)] = { 0x4000 };
    uint32_t acc_init[AME_ACC_WORDS];
    uint32_t acc_out[AME_ACC_WORDS];
    unsigned long i;

    memset(acc_out, 0, sizeof(acc_out));
    for (i = 0; i < AME_ACC_WORDS; i++) {
        acc_init[i] = 0x3f800000u + (uint32_t)i;
    }

    TEST_ASSERT(rownum >= 2, "configuration has inactive fp32 accumulator region");

    AME_INSN(MZERO8R(0));
    AME_INSN(MSETTILEMI(1));
    AME_INSN(MSETTILENI(1));
    AME_INSN(MSETTILEKI(1));

    load_tile16(0, tile_a);
    load_tile16(1, tile_bt);
    load_acc32(4, acc_init);

    AME_INSN(MFMACC_S_H(0, 1, 0));

    store_acc32(4, acc_out);

    TEST_ASSERT(acc_out[0] == 0x40400000u,
                "mfmacc.s.h updates active fp32 accumulator cell");

    for (i = 1; i < rownum; i++) {
        TEST_ASSERT(acc_out[i] == 0,
                    "mfmacc.s.h clears inactive fp32 accumulator columns");
    }

    for (i = rownum; i < rownum * rownum; i++) {
        TEST_ASSERT(acc_out[i] == 0,
                    "mfmacc.s.h clears inactive fp32 accumulator rows");
    }
}

static void test_mfmacc_h_clears_inactive_accumulator_region(void)
{
    const unsigned long tile_bytes = read_csr(CSR_XTLENB);
    const unsigned long row_bytes = read_csr(CSR_XTRLENB);
    const unsigned long rownum = tile_bytes / row_bytes;
    uint16_t tile_a[AME_TILE_BYTES / sizeof(uint16_t)] = { 0x3c00 };
    uint16_t tile_bt[AME_TILE_BYTES / sizeof(uint16_t)] = { 0x4000 };
    uint16_t acc_init[AME_ACC_WORDS * (sizeof(uint32_t) / sizeof(uint16_t))];
    uint16_t acc_out[AME_ACC_WORDS * (sizeof(uint32_t) / sizeof(uint16_t))];
    unsigned long i;

    memset(acc_out, 0, sizeof(acc_out));
    for (i = 0; i < sizeof(acc_init) / sizeof(acc_init[0]); i++) {
        acc_init[i] = (uint16_t)(0x3c00u + i);
    }

    TEST_ASSERT(rownum >= 2, "configuration has inactive fp16 accumulator region");

    AME_INSN(MZERO8R(0));
    AME_INSN(MSETTILEMI(1));
    AME_INSN(MSETTILENI(1));
    AME_INSN(MSETTILEKI(1));

    load_tile16(0, tile_a);
    load_tile16(1, tile_bt);
    load_acc16(4, acc_init);

    AME_INSN(MFMACC_H(0, 1, 0));

    store_acc16(4, acc_out);

    TEST_ASSERT(acc_out[0] == 0x4200u,
                "mfmacc.h updates active fp16 accumulator cell");

    for (i = 1; i < rownum; i++) {
        TEST_ASSERT(acc_out[i] == 0,
                    "mfmacc.h clears inactive fp16 accumulator columns");
    }

    for (i = rownum; i < rownum * rownum; i++) {
        TEST_ASSERT(acc_out[i] == 0,
                    "mfmacc.h clears inactive fp16 accumulator rows");
    }
}

int main(void)
{
    printf("=== AME MMACC Test ===\n");

    test_mmacc_operand_order();
    test_strided_load_uses_physical_row_stride();
    test_mmacc_whole_load_uses_physical_row_stride();
    test_mmacc_clears_inactive_accumulator_region();
    test_mfmacc_s_h_clears_inactive_accumulator_region();
    test_mfmacc_h_clears_inactive_accumulator_region();

    TEST_SUMMARY();
}