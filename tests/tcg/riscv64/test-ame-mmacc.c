/*
 * AME mmacc regression tests.
 *
 * Verifies `mmacc.w.b` operand orientation follows `A(ms1) * B_T(ms2)`.
 *
 * Copyright (c) 2026
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ame_insn.h"

enum {
    AME_TILE_BYTES = 128,
    AME_ACC_WORDS = 4096 / sizeof(int32_t),
};

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

int main(void)
{
    printf("=== AME MMACC Test ===\n");

    test_mmacc_operand_order();

    TEST_SUMMARY();
}