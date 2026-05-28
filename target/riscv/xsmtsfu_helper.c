/*
 * XSmtsfu helper implementations
 *
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include "qemu/osdep.h"
#include "qemu/host-utils.h"
#include "qemu/bitops.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "fpu/softfloat.h"
#include <math.h>

#define xsmtsfu_env_tlenb(env)  ame_cfg_tlenb(xsmtsfu_env_cfg(env))
#define xsmtsfu_env_cfg(env)    (&env_archcpu(env)->cfg)

static inline uint8_t *xsmtsfu_tile_ptr(CPURISCVState *env, uint32_t id)
{
    g_assert(id < AME_NR_TILES);
    return (uint8_t *)env->ame_tile + id * xsmtsfu_env_tlenb(env);
}

static inline float xsmtsfu_f32_to_host(uint32_t raw)
{
    union {
        uint32_t i;
        float f;
    } u = { .i = raw };

    return u.f;
}

static inline uint32_t xsmtsfu_host_to_f32(float f)
{
    union {
        float f;
        uint32_t i;
    } u = { .f = f };

    return u.i;
}

static void xsmtsfu_tile32(CPURISCVState *env, uint32_t md,
                           uint32_t ms2,
                           uint32_t (*op)(uint32_t, float_status *))
{
    uint32_t *dst = (uint32_t *)xsmtsfu_tile_ptr(env, md);
    const uint32_t *src = (const uint32_t *)xsmtsfu_tile_ptr(env, ms2);
    uint32_t elems = xsmtsfu_env_tlenb(env) / sizeof(uint32_t);
    float_status *fpst = &env->fp_status;
    uint32_t i;

    for (i = 0; i < elems; i++) {
        dst[i] = op(src[i], fpst);
    }
}

static uint32_t xsmtsfu_exp2(uint32_t raw, float_status *fpst)
{
    (void)fpst;
    return xsmtsfu_host_to_f32(exp2f(xsmtsfu_f32_to_host(raw)));
}

static uint32_t xsmtsfu_tanh(uint32_t raw, float_status *fpst)
{
    (void)fpst;
    return xsmtsfu_host_to_f32(tanhf(xsmtsfu_f32_to_host(raw)));
}

static uint32_t xsmtsfu_log2(uint32_t raw, float_status *fpst)
{
    (void)fpst;
    return xsmtsfu_host_to_f32(log2f(xsmtsfu_f32_to_host(raw)));
}

static uint32_t xsmtsfu_rcp(uint32_t raw, float_status *fpst)
{
    return float32_val(float32_div(make_float32(0x3f800000u),
                                   make_float32(raw), fpst));
}

void HELPER(xsmtsfu_mfex2_s)(CPURISCVState *env, uint32_t md, uint32_t ms2)
{
    xsmtsfu_tile32(env, md, ms2, xsmtsfu_exp2);
}

void HELPER(xsmtsfu_mftanh_s)(CPURISCVState *env, uint32_t md, uint32_t ms2)
{
    xsmtsfu_tile32(env, md, ms2, xsmtsfu_tanh);
}

void HELPER(xsmtsfu_mflg2_s)(CPURISCVState *env, uint32_t md, uint32_t ms2)
{
    xsmtsfu_tile32(env, md, ms2, xsmtsfu_log2);
}

void HELPER(xsmtsfu_mfrcp_s)(CPURISCVState *env, uint32_t md, uint32_t ms2)
{
    xsmtsfu_tile32(env, md, ms2, xsmtsfu_rcp);
}
