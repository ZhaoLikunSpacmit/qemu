/*
 * AME MISC regression tests.
 *
 * Copyright (c) 2025
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdint.h>
#include <stdio.h>

#include "ame_insn.h"

enum {
    REG_TILE0 = 0,
    REG_TILE1 = 1,
    REG_TILE2 = 2,
    REG_ACC0 = 4,
};

static inline void mmovb_store(unsigned reg, unsigned long idx,
                               unsigned long value)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVB_M_X(0, 10, 11)) : "memory");
        break;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVB_M_X(1, 10, 11)) : "memory");
        break;
    }
    case 2: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVB_M_X(2, 10, 11)) : "memory");
        break;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVB_M_X(4, 10, 11)) : "memory");
        break;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovb store register");
        break;
    }
}

static inline uint8_t mmovb_load(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 0, 10)), "r"(a0) : "memory");
        return (uint8_t)a2;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 1, 10)), "r"(a0) : "memory");
        return (uint8_t)a2;
    }
    case 2: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 2, 10)), "r"(a0) : "memory");
        return (uint8_t)a2;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 4, 10)), "r"(a0) : "memory");
        return (uint8_t)a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovb load register");
        return 0;
    }
}

static inline unsigned long mmovb_load_raw(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 0, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 1, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 2: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 2, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVB_X_M(12, 4, 10)), "r"(a0) : "memory");
        return a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovb raw load register");
        return 0;
    }
}

static inline void mmovh_store(unsigned reg, unsigned long idx,
                               unsigned long value)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVH_M_X(0, 10, 11)) : "memory");
        break;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVH_M_X(4, 10, 11)) : "memory");
        break;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovh store register");
        break;
    }
}

static inline uint16_t mmovh_load(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVH_X_M(12, 0, 10)), "r"(a0) : "memory");
        return (uint16_t)a2;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVH_X_M(12, 1, 10)), "r"(a0) : "memory");
        return (uint16_t)a2;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVH_X_M(12, 4, 10)), "r"(a0) : "memory");
        return (uint16_t)a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovh load register");
        return 0;
    }
}

static inline unsigned long mmovh_load_raw(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVH_X_M(12, 0, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVH_X_M(12, 1, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVH_X_M(12, 4, 10)), "r"(a0) : "memory");
        return a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovh raw load register");
        return 0;
    }
}

static inline void mmovw_store(unsigned reg, unsigned long idx,
                               unsigned long value)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVW_M_X(0, 10, 11)) : "memory");
        break;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVW_M_X(4, 10, 11)) : "memory");
        break;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovw store register");
        break;
    }
}

static inline uint32_t mmovw_load(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVW_X_M(12, 0, 10)), "r"(a0) : "memory");
        return (uint32_t)a2;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVW_X_M(12, 1, 10)), "r"(a0) : "memory");
        return (uint32_t)a2;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVW_X_M(12, 4, 10)), "r"(a0) : "memory");
        return (uint32_t)a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovw load register");
        return 0;
    }
}

static inline unsigned long mmovw_load_raw(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 0: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVW_X_M(12, 0, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 1: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVW_X_M(12, 1, 10)), "r"(a0) : "memory");
        return a2;
    }
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVW_X_M(12, 4, 10)), "r"(a0) : "memory");
        return a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovw raw load register");
        return 0;
    }
}

static inline void mmovd_store(unsigned reg, unsigned long idx,
                               unsigned long value)
{
    switch (reg) {
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a1 __asm__("a1") = value;
        __asm__ volatile (".4byte %2" :: "r"(a0), "r"(a1),
                          "i"(MMOVD_M_X(4, 10, 11)) : "memory");
        break;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovd store register");
        break;
    }
}

static inline uint64_t mmovd_load(unsigned reg, unsigned long idx)
{
    switch (reg) {
    case 4: {
        register unsigned long a0 __asm__("a0") = idx;
        register unsigned long a2 __asm__("a2");
        __asm__ volatile (".4byte %1" : "=r"(a2) :
                          "i"(MMOVD_X_M(12, 4, 10)), "r"(a0) : "memory");
        return (uint64_t)a2;
    }
    default:
        TEST_ASSERT(0, "unsupported mmovd load register");
        return 0;
    }
}

static void clear_all(void)
{
    AME_INSN(MZERO8R(0));
}

static void test_mmov_cross_class_undisturbed(void)
{
    int row;
    int col;

    clear_all();
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 128; col++) {
            mmovb_store(REG_ACC0, row * 128 + col, 0xaa);
        }
    }
    for (row = 0; row < 32; row++) {
        for (col = 0; col < 4; col++) {
            mmovb_store(REG_TILE0, row * 4 + col,
                        (uint8_t)(((row * 4 + col) ^ 0x5a) & 0xff));
        }
    }

    AME_INSN(MMOV_MM(REG_ACC0, REG_TILE0));

    for (row = 0; row < 32; row++) {
        for (col = 0; col < 4; col++) {
            uint8_t got = mmovb_load(REG_ACC0, row * 128 + col);
            uint8_t expect = (uint8_t)(((row * 4 + col) ^ 0x5a) & 0xff);

            TEST_ASSERT(got == expect,
                        "mmov.mm tile->acc copied source row prefix");
        }
        for (col = 4; col < 128; col++) {
            uint8_t got = mmovb_load(REG_ACC0, row * 128 + col);

            TEST_ASSERT(got == 0xaa,
                        "mmov.mm tile->acc kept remaining acc row bytes undisturbed");
        }
    }
}

static void test_mmov_scalar_widths(void)
{
    clear_all();

    mmovb_store(REG_TILE0, 5, 0x7e);
    TEST_ASSERT(mmovb_load(REG_TILE0, 5) == 0x7e,
                "mmovb.m.x / mmovb.x.m roundtrip");

    mmovh_store(REG_TILE0, 2, 0x1234);
    TEST_ASSERT(mmovh_load(REG_TILE0, 2) == 0x1234,
                "mmovh.m.x / mmovh.x.m roundtrip");

    mmovw_store(REG_ACC0, 3, 0x12345678u);
    TEST_ASSERT(mmovw_load(REG_ACC0, 3) == 0x12345678u,
                "mmovw.m.x / mmovw.x.m roundtrip");

    mmovd_store(REG_ACC0, 1, 0x1122334455667788ull);
    TEST_ASSERT(mmovd_load(REG_ACC0, 1) == 0x1122334455667788ull,
                "mmovd.m.x / mmovd.x.m roundtrip");

    mmovb_store(REG_TILE0, 7, 0xfeu);
    TEST_ASSERT((long)mmovb_load_raw(REG_TILE0, 7) == -2L,
                "mmovb.x.m sign-extends to XLEN");

    mmovh_store(REG_TILE0, 9, 0xff80u);
    TEST_ASSERT((long)mmovh_load_raw(REG_TILE0, 9) == -128L,
                "mmovh.x.m sign-extends to XLEN");

    mmovw_store(REG_ACC0, 11, 0x80000001u);
    TEST_ASSERT((long)mmovw_load_raw(REG_ACC0, 11) == -2147483647L,
                "mmovw.x.m sign-extends to XLEN");
}

static void test_sfu_fp32_tile_ops(void)
{
    clear_all();

    mmovw_store(REG_TILE0, 0, 0x00000000u); /* +0.0 */
    mmovw_store(REG_TILE0, 1, 0x3f800000u); /* +1.0 */
    mmovw_store(REG_TILE0, 2, 0x40000000u); /* +2.0 */

    AME_INSN(VFEX2_V(REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovw_load(REG_TILE1, 0) == 0x3f800000u,
                "vfex2.v 2^0 == 1");
    TEST_ASSERT(mmovw_load(REG_TILE1, 1) == 0x40000000u,
                "vfex2.v 2^1 == 2");

    AME_INSN(VFTANH_V(REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovw_load(REG_TILE1, 0) == 0x00000000u,
                "vftanh.v tanh(0) == 0");

    AME_INSN(VFLG2_V(REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovw_load(REG_TILE1, 1) == 0x00000000u,
                "vflg2.v log2(1) == 0");
    TEST_ASSERT(mmovw_load(REG_TILE1, 2) == 0x3f800000u,
                "vflg2.v log2(2) == 1");

    AME_INSN(VFRCP_V(REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovw_load(REG_TILE1, 1) == 0x3f800000u,
                "vfrcp.v 1/1 == 1");
    TEST_ASSERT(mmovw_load(REG_TILE1, 2) == 0x3f000000u,
                "vfrcp.v 1/2 == 0.5");
}

static void init_tile_rows_for_pack(void)
{
    static const uint8_t tile0[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    static const uint8_t tile1[] = { 10, 11, 12, 13, 14, 15, 16, 17 };
    int i;

    clear_all();
    for (i = 0; i < 8; i++) {
        mmovb_store(REG_TILE0, i, tile0[i]);
        mmovb_store(REG_TILE1, i, tile1[i]);
    }
}

static void test_mpack_tile(void)
{
    init_tile_rows_for_pack();
    AME_INSN(MPACK(REG_TILE2, REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovb_load(REG_TILE2, 0) == 1 &&
                mmovb_load(REG_TILE2, 1) == 2 &&
                mmovb_load(REG_TILE2, 2) == 10 &&
                mmovb_load(REG_TILE2, 3) == 11,
                "mpack low+low row0");
    TEST_ASSERT(mmovb_load(REG_TILE2, 4) == 5 &&
                mmovb_load(REG_TILE2, 5) == 6 &&
                mmovb_load(REG_TILE2, 6) == 14 &&
                mmovb_load(REG_TILE2, 7) == 15,
                "mpack low+low row1");

    init_tile_rows_for_pack();
    AME_INSN(MPACKHL(REG_TILE2, REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovb_load(REG_TILE2, 0) == 3 &&
                mmovb_load(REG_TILE2, 1) == 4 &&
                mmovb_load(REG_TILE2, 2) == 10 &&
                mmovb_load(REG_TILE2, 3) == 11,
                "mpackhl high+low row0");

    init_tile_rows_for_pack();
    AME_INSN(MPACKHH(REG_TILE2, REG_TILE1, REG_TILE0));
    TEST_ASSERT(mmovb_load(REG_TILE2, 0) == 3 &&
                mmovb_load(REG_TILE2, 1) == 4 &&
                mmovb_load(REG_TILE2, 2) == 12 &&
                mmovb_load(REG_TILE2, 3) == 13,
                "mpackhh high+high row0");
}

static void init_tile_rows_for_row_slide(void)
{
    int i;

    clear_all();
    for (i = 0; i < 16; i++) {
        mmovb_store(REG_TILE0, i, (uint8_t)(i + 1));
    }
}

static void test_mrslide_tile(void)
{
    init_tile_rows_for_row_slide();
    AME_INSN(MRSLIDEDOWN(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovb_load(REG_TILE1, 0) == 0 &&
                mmovb_load(REG_TILE1, 1) == 0 &&
                mmovb_load(REG_TILE1, 2) == 0 &&
                mmovb_load(REG_TILE1, 3) == 0,
                "mrslidedown zero-fills top row");
    TEST_ASSERT(mmovb_load(REG_TILE1, 4) == 1 &&
                mmovb_load(REG_TILE1, 5) == 2 &&
                mmovb_load(REG_TILE1, 6) == 3 &&
                mmovb_load(REG_TILE1, 7) == 4,
                "mrslidedown shifts row data down");

    init_tile_rows_for_row_slide();
    AME_INSN(MRSLIDEUP(REG_TILE1, REG_TILE0, 2));
    TEST_ASSERT(mmovb_load(REG_TILE1, 0) == 9 &&
                mmovb_load(REG_TILE1, 1) == 10 &&
                mmovb_load(REG_TILE1, 2) == 11 &&
                mmovb_load(REG_TILE1, 3) == 12,
                "mrslideup shifts row data up");
    TEST_ASSERT(mmovb_load(REG_TILE1, 8) == 0 &&
                mmovb_load(REG_TILE1, 9) == 0 &&
                mmovb_load(REG_TILE1, 10) == 0 &&
                mmovb_load(REG_TILE1, 11) == 0,
                "mrslideup zero-fills bottom rows");
}

static void test_mcslide_tile(void)
{
    clear_all();
    mmovb_store(REG_TILE0, 0, 1);
    mmovb_store(REG_TILE0, 1, 2);
    mmovb_store(REG_TILE0, 2, 3);
    mmovb_store(REG_TILE0, 3, 4);

    AME_INSN(MCSLIDEDOWN_B(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovb_load(REG_TILE1, 0) == 0 &&
                mmovb_load(REG_TILE1, 1) == 1 &&
                mmovb_load(REG_TILE1, 2) == 2 &&
                mmovb_load(REG_TILE1, 3) == 3,
                "mcslidedown.b shifts columns toward higher indices");

    AME_INSN(MCSLIDEUP_B(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovb_load(REG_TILE1, 0) == 2 &&
                mmovb_load(REG_TILE1, 1) == 3 &&
                mmovb_load(REG_TILE1, 2) == 4 &&
                mmovb_load(REG_TILE1, 3) == 0,
                "mcslideup.b shifts columns toward lower indices");

    clear_all();
    mmovh_store(REG_TILE0, 0, 0x1111);
    mmovh_store(REG_TILE0, 1, 0x2222);
    AME_INSN(MCSLIDEDOWN_H(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovh_load(REG_TILE1, 0) == 0 &&
                mmovh_load(REG_TILE1, 1) == 0x1111,
                "mcslidedown.h shifts halfword columns");

    AME_INSN(MCSLIDEUP_H(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovh_load(REG_TILE1, 0) == 0x2222 &&
                mmovh_load(REG_TILE1, 1) == 0,
                "mcslideup.h shifts halfword columns");

    clear_all();
    mmovw_store(REG_TILE0, 0, 0xdeadbeefu);
    AME_INSN(MCSLIDEDOWN_W(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovw_load(REG_TILE1, 0) == 0,
                "mcslidedown.w zero-fills single-column tile rows");

    AME_INSN(MCSLIDEUP_W(REG_TILE1, REG_TILE0, 1));
    TEST_ASSERT(mmovw_load(REG_TILE1, 0) == 0,
                "mcslideup.w zero-fills single-column tile rows");
}

int main(void)
{
    printf("=== AME MISC Test ===\n");

    test_mmov_cross_class_undisturbed();
    test_mmov_scalar_widths();
    test_sfu_fp32_tile_ops();
    test_mpack_tile();
    test_mrslide_tile();
    test_mcslide_tile();

    TEST_SUMMARY();
}