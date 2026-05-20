/*
 * AME (Accelerated Matrix Extension) helper implementations
 *
 * Copyright (c) 2025
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include "qemu/osdep.h"
#include "qemu/host-utils.h"
#include "qemu/bitops.h"
#include "cpu.h"
#include "accel/tcg/cpu-ldst.h"
#include "exec/memop.h"
#include "exec/page-protection.h"
#include "exec/helper-proto.h"
#include "fpu/softfloat.h"
#include "internals.h"

/*
 * ──────────────────────────────────────────
 *  AME runtime dimension helpers
 * ──────────────────────────────────────────
 * Use these instead of the compile-time AME_TILE_LEN_B / AME_ACC_LEN_B
 * constants whenever the actual (possibly narrower) register size is needed.
 */
#define ame_env_tlenb(env)      ame_cfg_tlenb(ame_env_cfg(env))
#define ame_env_trlenb(env)     ame_cfg_trlenb(ame_env_cfg(env))
#define ame_env_rownum(env)     ame_cfg_rownum(ame_env_cfg(env))
#define ame_env_acc_len_b(env)  ame_cfg_acc_len_b(ame_env_cfg(env))
#define ame_env_cfg(env)        (&env_archcpu(env)->cfg)

/*
 * ──────────────────────────────────────────
 *  Internal helpers: register base pointers
 * ──────────────────────────────────────────
 */

/* Return pointer to the start of tile[id] inside CPURISCVState */
static inline uint8_t *xsmtame06v_tile_ptr(CPURISCVState *env, uint32_t id)
{
    g_assert(id < AME_NR_TILES);
    return (uint8_t *)env->ame_tile + id * ame_env_tlenb(env);
}

/* Return pointer to the start of acc[id] inside CPURISCVState */
static inline uint8_t *xsmtame06v_acc_ptr(CPURISCVState *env, uint32_t id)
{
    g_assert(id < AME_NR_ACCS);
    return (uint8_t *)env->ame_acc + id * ame_env_acc_len_b(env);
}

typedef struct AMEShapeInfo {
    uint32_t m;
    uint32_t n;
    uint32_t k;
} AMEShapeInfo;

static inline AMEShapeInfo xsmtame06v_shape(CPURISCVState *env)
{
    return (AMEShapeInfo) {
        .m = env->mtilem,
        .n = env->mtilen,
        .k = env->mtilek,
    };
}

static inline uint16_t *xsmtame06v_tile16_ptr(CPURISCVState *env, uint32_t id)
{
    return (uint16_t *)xsmtame06v_tile_ptr(env, id);
}

static inline uint32_t *xsmtame06v_tile32_ptr(CPURISCVState *env, uint32_t id)
{
    return (uint32_t *)xsmtame06v_tile_ptr(env, id);
}

static inline int8_t *xsmtame06v_tile8s_ptr(CPURISCVState *env, uint32_t id)
{
    return (int8_t *)xsmtame06v_tile_ptr(env, id);
}

static inline uint8_t *xsmtame06v_acc8_ptr(CPURISCVState *env, uint32_t id)
{
    return xsmtame06v_acc_ptr(env, id);
}

static inline uint16_t *xsmtame06v_acc16_ptr(CPURISCVState *env, uint32_t id)
{
    return (uint16_t *)xsmtame06v_acc_ptr(env, id);
}

static inline uint32_t *xsmtame06v_acc32_ptr(CPURISCVState *env, uint32_t id)
{
    return (uint32_t *)xsmtame06v_acc_ptr(env, id);
}

static inline uint8_t *xsmtame06v_matrix_ptr(CPURISCVState *env, uint32_t reg,
                                               size_t *size)
{
    if (reg < AME_NR_TILES) {
        if (size) {
            *size = ame_env_tlenb(env);
        }
        return xsmtame06v_tile_ptr(env, reg);
    }

    g_assert(reg < AME_NR_TILES + AME_NR_ACCS);
    if (size) {
        *size = ame_env_acc_len_b(env);
    }
    return xsmtame06v_acc_ptr(env, reg - AME_NR_TILES);
}

static inline size_t xsmtame06v_mmov_elem_offset(size_t reg_size,
                                                   size_t elem_size,
                                                   target_ulong idx)
{
    size_t elem_count = reg_size / elem_size;
    size_t elem_idx = (size_t)(idx % elem_count);

    return elem_idx * elem_size;
}

static inline size_t xsmtame06v_matrix_row_bytes(CPURISCVState *env,
                                                   uint32_t reg)
{
    /* tile row = TRLEN/8 bytes; acc row = (ELEN/8)*ROWNUM bytes */
    return reg < AME_NR_TILES ?
           ame_env_trlenb(env) :
           ame_env_acc_len_b(env) / ame_env_rownum(env);
}

static inline size_t xsmtame06v_matrix_col_count(CPURISCVState *env,
                                                   uint32_t reg,
                                                   size_t elem_size)
{
    size_t row_bytes = xsmtame06v_matrix_row_bytes(env, reg);

    g_assert(row_bytes % elem_size == 0);
    return row_bytes / elem_size;
}

static void xsmtame06v_mmov_m_x_common(CPURISCVState *env, uint32_t md,
                                         target_ulong idx,
                                         target_ulong value,
                                         size_t elem_size)
{
    size_t reg_size;
    uint8_t *dst = xsmtame06v_matrix_ptr(env, md, &reg_size);
    size_t offset = xsmtame06v_mmov_elem_offset(reg_size, elem_size, idx);

    switch (elem_size) {
    case 1:
        stb_p(dst + offset, value);
        break;
    case 2:
        stw_le_p(dst + offset, value);
        break;
    case 4:
        stl_le_p(dst + offset, value);
        break;
    case 8:
        stq_le_p(dst + offset, value);
        break;
    default:
        g_assert_not_reached();
    }
}

static inline void xsmtame06v_load_tile8_stride(uint8_t *tile,
                                                  CPURISCVState *env,
                                                  target_ulong addr,
                                                  target_ulong stride,
                                                  uint32_t rows,
                                                  uint32_t cols,
                                                  bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            tile[row * cols + col] =
                cpu_ldub_data(env, addr + (transpose ?
                              col * stride + row :
                              row * stride + col));
        }
    }
}

static inline void xsmtame06v_store_tile8_stride(const uint8_t *tile,
                                                   CPURISCVState *env,
                                                   target_ulong addr,
                                                   target_ulong stride,
                                                   uint32_t rows,
                                                   uint32_t cols,
                                                   bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            cpu_stb_data(env, addr + (transpose ?
                         col * stride + row :
                         row * stride + col),
                         tile[row * cols + col]);
        }
    }
}

static inline void xsmtame06v_load_tile16_stride(uint16_t *tile16,
                                                   CPURISCVState *env,
                                                   target_ulong addr,
                                                   target_ulong stride,
                                                   uint32_t rows,
                                                   uint32_t cols,
                                                   bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            tile16[row * cols + col] =
                cpu_lduw_data(env, addr + (transpose ?
                              col * stride + row * sizeof(*tile16) :
                              row * stride + col * sizeof(*tile16)));
        }
    }
}

static inline void xsmtame06v_store_tile16_stride(const uint16_t *tile16,
                                                    CPURISCVState *env,
                                                    target_ulong addr,
                                                    target_ulong stride,
                                                    uint32_t rows,
                                                    uint32_t cols,
                                                    bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            cpu_stw_data(env, addr + (transpose ?
                         col * stride + row * sizeof(*tile16) :
                         row * stride + col * sizeof(*tile16)),
                         tile16[row * cols + col]);
        }
    }
}

static inline void xsmtame06v_load_tile32_stride(uint32_t *tile32,
                                                   CPURISCVState *env,
                                                   target_ulong addr,
                                                   target_ulong stride,
                                                   uint32_t rows,
                                                   uint32_t cols,
                                                   bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            tile32[row * cols + col] =
                cpu_ldl_data(env, addr + (transpose ?
                             col * stride + row * sizeof(*tile32) :
                             row * stride + col * sizeof(*tile32)));
        }
    }
}

static inline void xsmtame06v_store_tile32_stride(const uint32_t *tile32,
                                                    CPURISCVState *env,
                                                    target_ulong addr,
                                                    target_ulong stride,
                                                    uint32_t rows,
                                                    uint32_t cols,
                                                    bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            cpu_stl_data(env, addr + (transpose ?
                         col * stride + row * sizeof(*tile32) :
                         row * stride + col * sizeof(*tile32)),
                         tile32[row * cols + col]);
        }
    }
}

static inline void xsmtame06v_load_acc8_stride(uint8_t *acc8,
                                                 CPURISCVState *env,
                                                 target_ulong addr,
                                                 target_ulong stride,
                                                 uint32_t rows,
                                                 uint32_t cols,
                                                 bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            acc8[row * cols + col] =
                cpu_ldub_data(env, addr + (transpose ?
                              col * stride + row :
                              row * stride + col));
        }
    }
}

static inline void xsmtame06v_store_acc8_stride(const uint8_t *acc8,
                                                  CPURISCVState *env,
                                                  target_ulong addr,
                                                  target_ulong stride,
                                                  uint32_t rows,
                                                  uint32_t cols,
                                                  bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            cpu_stb_data(env, addr + (transpose ?
                         col * stride + row :
                         row * stride + col),
                         acc8[row * cols + col]);
        }
    }
}

static inline void xsmtame06v_load_acc16_stride(uint16_t *acc16,
                                                  CPURISCVState *env,
                                                  target_ulong addr,
                                                  target_ulong stride,
                                                  uint32_t rows,
                                                  uint32_t cols,
                                                  bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            acc16[row * cols + col] =
                cpu_lduw_data(env, addr + (transpose ?
                              col * stride + row * sizeof(*acc16) :
                              row * stride + col * sizeof(*acc16)));
        }
    }
}

static inline void xsmtame06v_store_acc16_stride(const uint16_t *acc16,
                                                   CPURISCVState *env,
                                                   target_ulong addr,
                                                   target_ulong stride,
                                                   uint32_t rows,
                                                   uint32_t cols,
                                                   bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            cpu_stw_data(env, addr + (transpose ?
                         col * stride + row * sizeof(*acc16) :
                         row * stride + col * sizeof(*acc16)),
                         acc16[row * cols + col]);
        }
    }
}

static inline void xsmtame06v_load_acc32_stride(uint32_t *acc32,
                                                  CPURISCVState *env,
                                                  target_ulong addr,
                                                  target_ulong stride,
                                                  uint32_t rows,
                                                  uint32_t cols,
                                                  bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            acc32[row * cols + col] =
                cpu_ldl_data(env, addr + (transpose ?
                              col * stride + row * sizeof(*acc32) :
                              row * stride + col * sizeof(*acc32)));
        }
    }
}

static inline void xsmtame06v_store_acc32_stride(const uint32_t *acc32,
                                                   CPURISCVState *env,
                                                   target_ulong addr,
                                                   target_ulong stride,
                                                   uint32_t rows,
                                                   uint32_t cols,
                                                   bool transpose)
{
    uint32_t row, col;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            cpu_stl_data(env, addr + (transpose ?
                         col * stride + row * sizeof(*acc32) :
                         row * stride + col * sizeof(*acc32)),
                         acc32[row * cols + col]);
        }
    }
}

/*
 * ──────────────────────────────────────────
 *  Load helpers
 * ──────────────────────────────────────────
 *
 * Legacy helpers still support stride where required by old instructions.
 * mlme16/mlme32/msme16/msme32 are contiguous.
 * mlae16/mlbe16 use explicit stride without transpose.
 * mlate16/mlbte16 use explicit stride and transpose while loading.
 * msae16/msbe16 use explicit stride without transpose while storing.
 * msate16/msbte16 use explicit stride and transpose while storing.
 */

void HELPER(xsmtame06v_mlme8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_load_tile8_stride(tile, env, addr, stride,
                                   shape.m, shape.k, false);
}

void HELPER(xsmtame06v_mlae8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_load_tile8_stride(tile, env, addr, stride,
                                   shape.m, shape.k, false);
}

void HELPER(xsmtame06v_mlbe8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_load_tile8_stride(tile, env, addr, stride,
                                   shape.n, shape.k, false);
}

void HELPER(xsmtame06v_mlate8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_load_tile8_stride(tile, env, addr, stride,
                                   shape.m, shape.k, true);
}

void HELPER(xsmtame06v_mlbte8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_load_tile8_stride(tile, env, addr, stride,
                                   shape.n, shape.k, true);
}

void HELPER(xsmtame06v_mlme16)(CPURISCVState *env, uint32_t td,
                        target_ulong addr)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_load_tile16_stride(tile16, env, addr,
                                    shape.k * sizeof(*tile16),
                                    shape.m, shape.k, false);
}

void HELPER(xsmtame06v_mlae16)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_load_tile16_stride(tile16, env, addr, stride,
                                    shape.m, shape.k, false);
}

void HELPER(xsmtame06v_mlae32)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_load_tile32_stride(tile32, env, addr, stride,
                                    shape.m, shape.k, false);
}

void HELPER(xsmtame06v_mlbe16)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_load_tile16_stride(tile16, env, addr, stride,
                                    shape.n, shape.k, false);
}

void HELPER(xsmtame06v_mlbe32)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_load_tile32_stride(tile32, env, addr, stride,
                                    shape.n, shape.k, false);
}

void HELPER(xsmtame06v_mlate16)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_load_tile16_stride(tile16, env, addr, stride,
                                    shape.m, shape.k, true);
}

void HELPER(xsmtame06v_mlate32)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_load_tile32_stride(tile32, env, addr, stride,
                                    shape.m, shape.k, true);
}

void HELPER(xsmtame06v_mlbte16)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_load_tile16_stride(tile16, env, addr, stride,
                                    shape.n, shape.k, true);
}

void HELPER(xsmtame06v_mlbte32)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_load_tile32_stride(tile32, env, addr, stride,
                                    shape.n, shape.k, true);
}

void HELPER(xsmtame06v_mlce8)(CPURISCVState *env, uint32_t ad,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *acc8 = xsmtame06v_acc8_ptr(env, ad);

    xsmtame06v_load_acc8_stride(acc8, env, addr, stride,
                                  shape.m, shape.n, false);
}

void HELPER(xsmtame06v_mlce16)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *acc16 = xsmtame06v_acc16_ptr(env, ad);

    xsmtame06v_load_acc16_stride(acc16, env, addr, stride,
                                   shape.m, shape.n, false);
}

void HELPER(xsmtame06v_mlce32)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *acc32 = xsmtame06v_acc32_ptr(env, ad);

    xsmtame06v_load_acc32_stride(acc32, env, addr, stride,
                                   shape.m, shape.n, false);
}

void HELPER(xsmtame06v_mlcte8)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *acc8 = xsmtame06v_acc8_ptr(env, ad);

    xsmtame06v_load_acc8_stride(acc8, env, addr, stride,
                                  shape.m, shape.n, true);
}

void HELPER(xsmtame06v_mlcte16)(CPURISCVState *env, uint32_t ad,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint16_t *acc16 = xsmtame06v_acc16_ptr(env, ad);

    xsmtame06v_load_acc16_stride(acc16, env, addr, stride,
                                   shape.m, shape.n, true);
}

void HELPER(xsmtame06v_mlcte32)(CPURISCVState *env, uint32_t ad,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *acc32 = xsmtame06v_acc32_ptr(env, ad);

    xsmtame06v_load_acc32_stride(acc32, env, addr, stride,
                                   shape.m, shape.n, true);
}

void HELPER(xsmtame06v_mlme32)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *acc32 = xsmtame06v_acc32_ptr(env, ad);

    xsmtame06v_load_acc32_stride(acc32, env, addr,
                                   shape.n * sizeof(*acc32),
                                   shape.m, shape.n, false);
}

/*
 * ──────────────────────────────────────────
 *  Store helpers
 * ──────────────────────────────────────────
 */

void HELPER(xsmtame06v_msae16)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_store_tile16_stride(tile16, env, addr, stride,
                                     shape.m, shape.k, false);
}

void HELPER(xsmtame06v_msae32)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_store_tile32_stride(tile32, env, addr, stride,
                                     shape.m, shape.k, false);
}

void HELPER(xsmtame06v_msae8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_store_tile8_stride(tile, env, addr, stride,
                                    shape.m, shape.k, false);
}

void HELPER(xsmtame06v_msbe16)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_store_tile16_stride(tile16, env, addr, stride,
                                     shape.n, shape.k, false);
}

void HELPER(xsmtame06v_msbe32)(CPURISCVState *env, uint32_t td,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_store_tile32_stride(tile32, env, addr, stride,
                                     shape.n, shape.k, false);
}

void HELPER(xsmtame06v_msbe8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_store_tile8_stride(tile, env, addr, stride,
                                    shape.n, shape.k, false);
}

void HELPER(xsmtame06v_msce8)(CPURISCVState *env, uint32_t ad,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *acc8 = xsmtame06v_acc8_ptr(env, ad);

    xsmtame06v_store_acc8_stride(acc8, env, addr, stride,
                                   shape.m, shape.n, false);
}

void HELPER(xsmtame06v_msce16)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *acc16 = xsmtame06v_acc16_ptr(env, ad);

    xsmtame06v_store_acc16_stride(acc16, env, addr, stride,
                                    shape.m, shape.n, false);
}

void HELPER(xsmtame06v_msce32)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint32_t *acc32 = xsmtame06v_acc32_ptr(env, ad);

    xsmtame06v_store_acc32_stride(acc32, env, addr, stride,
                                    shape.m, shape.n, false);
}

void HELPER(xsmtame06v_msate8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_store_tile8_stride(tile, env, addr, stride,
                                    shape.m, shape.k, true);
}

void HELPER(xsmtame06v_msbte8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_store_tile8_stride(tile, env, addr, stride,
                                    shape.n, shape.k, true);
}

void HELPER(xsmtame06v_mscte8)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *acc8 = xsmtame06v_acc8_ptr(env, ad);

    xsmtame06v_store_acc8_stride(acc8, env, addr, stride,
                                   shape.m, shape.n, true);
}

void HELPER(xsmtame06v_mscte16)(CPURISCVState *env, uint32_t ad,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *acc16 = xsmtame06v_acc16_ptr(env, ad);

    xsmtame06v_store_acc16_stride(acc16, env, addr, stride,
                                    shape.m, shape.n, true);
}

void HELPER(xsmtame06v_mscte32)(CPURISCVState *env, uint32_t ad,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint32_t *acc32 = xsmtame06v_acc32_ptr(env, ad);

    xsmtame06v_store_acc32_stride(acc32, env, addr, stride,
                                    shape.m, shape.n, true);
}

void HELPER(xsmtame06v_msate16)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_store_tile16_stride(tile16, env, addr, stride,
                                     shape.m, shape.k, true);
}

void HELPER(xsmtame06v_msate32)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_store_tile32_stride(tile32, env, addr, stride,
                                     shape.m, shape.k, true);
}

void HELPER(xsmtame06v_msbte16)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_store_tile16_stride(tile16, env, addr, stride,
                                     shape.n, shape.k, true);
}

void HELPER(xsmtame06v_msbte32)(CPURISCVState *env, uint32_t td,
                         target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint32_t *tile32 = xsmtame06v_tile32_ptr(env, td);

    xsmtame06v_store_tile32_stride(tile32, env, addr, stride,
                                     shape.n, shape.k, true);
}

void HELPER(xsmtame06v_msme8)(CPURISCVState *env, uint32_t td,
                       target_ulong addr, target_ulong stride)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint8_t *tile = xsmtame06v_tile_ptr(env, td);

    xsmtame06v_store_tile8_stride(tile, env, addr, stride,
                                    shape.m, shape.k, false);
}

void HELPER(xsmtame06v_msme16)(CPURISCVState *env, uint32_t td,
                        target_ulong addr)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tile16 = xsmtame06v_tile16_ptr(env, td);

    xsmtame06v_store_tile16_stride(tile16, env, addr,
                                     shape.k * sizeof(*tile16),
                                     shape.m, shape.k, false);
}

void HELPER(xsmtame06v_msme32)(CPURISCVState *env, uint32_t ad,
                        target_ulong addr)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    uint32_t *acc32 = xsmtame06v_acc32_ptr(env, ad);

    xsmtame06v_store_acc32_stride(acc32, env, addr,
                                    shape.n * sizeof(*acc32),
                                    shape.m, shape.n, false);
}

/*
 * ──────────────────────────────────────────
 *  GEMM helpers
 * ──────────────────────────────────────────
 *
 * INT8 → INT32 GEMM  (mmacc.w.b):
 *   acc[ad][m][n] += Σ_{k} (int32)tile_A[ts2][m][k] * (int32)tile_B_T[ts1][n][k]
 *
 * tile_B is stored transposed: shape [N][K], so tile_B_T[n][k] is natural.
 *
 * FP16 → FP32 GEMM  (mfmacc.s.h):
 *   mfmacc.s.h md, ms2, ms1
 *   acc[md][m][n] += Σ_{k} fp32(A[ms1][m][k]) * fp32(B_T[ms2][n][k])
 */

static void xsmtame06v_mmacc_w_b_common(CPURISCVState *env, uint32_t ad,
                                          uint32_t ts2, uint32_t ts1,
                                          bool lhs_unsigned,
                                          bool rhs_unsigned)
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tA = (const uint8_t *)xsmtame06v_tile8s_ptr(env, ts2);
    const uint8_t *tBT = (const uint8_t *)xsmtame06v_tile8s_ptr(env, ts1);
    int32_t *acc = (int32_t *)xsmtame06v_acc32_ptr(env, ad);
    uint32_t m, n, k;

    for (m = 0; m < shape.m; m++) {
        for (n = 0; n < shape.n; n++) {
            int32_t sum = 0;
            for (k = 0; k < shape.k; k++) {
                uint32_t a_raw = tA[m * shape.k + k];
                uint32_t b_raw = tBT[n * shape.k + k];
                int32_t a = lhs_unsigned ? (int32_t)a_raw : (int32_t)(int8_t)a_raw;
                int32_t b = rhs_unsigned ? (int32_t)b_raw : (int32_t)(int8_t)b_raw;

                sum += a * b;
            }
            acc[m * shape.n + n] += sum;
        }
    }
}

void HELPER(xsmtame06v_mmaccu_w_b)(CPURISCVState *env, uint32_t ad,
                            uint32_t ts2, uint32_t ts1)
{
    xsmtame06v_mmacc_w_b_common(env, ad, ts2, ts1, true, true);
}

void HELPER(xsmtame06v_mmaccus_w_b)(CPURISCVState *env, uint32_t ad,
                             uint32_t ts2, uint32_t ts1)
{
    xsmtame06v_mmacc_w_b_common(env, ad, ts2, ts1, true, false);
}

void HELPER(xsmtame06v_mmaccsu_w_b)(CPURISCVState *env, uint32_t ad,
                             uint32_t ts2, uint32_t ts1)
{
    xsmtame06v_mmacc_w_b_common(env, ad, ts2, ts1, false, true);
}

void HELPER(xsmtame06v_mmacc_w_b)(CPURISCVState *env, uint32_t ad,
                           uint32_t ts2, uint32_t ts1)
{
    xsmtame06v_mmacc_w_b_common(env, ad, ts2, ts1, false, false);
}

static inline float32 xsmtame06v_mfmacc_fp16_to_f32(uint16_t raw,
                                                       float_status *fpst)
{
    return float16_to_float32(make_float16(raw), true, fpst);
}

static inline float32 xsmtame06v_mfmacc_bf16_to_f32(uint16_t raw,
                                                       float_status *fpst)
{
    return bfloat16_to_float32((bfloat16)raw, fpst);
}

static inline uint16_t xsmtame06v_mfmacc_f32_to_f16_bits(float32 raw,
                                                           float_status *fpst)
{
    return float16_val(float32_to_float16(raw, true, fpst));
}

static inline uint16_t xsmtame06v_mfmacc_f32_to_bf16_bits(float32 raw,
                                                            float_status *fpst)
{
    return (uint16_t)float32_to_bfloat16(raw, fpst);
}

static inline float32 xsmtame06v_mfmacc_fp8_to_f32(uint8_t raw,
                                                     uint8_t exp_bits,
                                                     uint8_t frac_bits,
                                                     int16_t exp_bias,
                                                     float_status *fpst)
{
    uint8_t exp_mask = (1u << exp_bits) - 1;
    uint8_t frac_mask = (1u << frac_bits) - 1;
    bool sign = raw >> (exp_bits + frac_bits);
    uint8_t exp = (raw >> frac_bits) & exp_mask;
    uint8_t frac = raw & frac_mask;
    int16_t unbiased_exp;
    int32_t sig;

    if (exp == exp_mask) {
        if (frac) {
            return float32_default_nan(fpst);
        }
        return make_float32((sign ? 0x80000000u : 0) | 0x7f800000u);
    }

    if (!exp) {
        if (!frac) {
            return make_float32(sign ? 0x80000000u : 0);
        }
        unbiased_exp = 1 - exp_bias;
        sig = frac;
    } else {
        unbiased_exp = exp - exp_bias;
        sig = (1u << frac_bits) | frac;
    }

    return int32_to_float32_scalbn(sign ? -sig : sig,
                                  unbiased_exp - frac_bits, fpst);
}

static inline float32 xsmtame06v_mfmacc_e4_to_f32(uint8_t raw,
                                                     float_status *fpst)
{
    return xsmtame06v_mfmacc_fp8_to_f32(raw, 4, 3, 7, fpst);
}

static inline float32 xsmtame06v_mfmacc_e5_to_f32(uint8_t raw,
                                                     float_status *fpst)
{
    return xsmtame06v_mfmacc_fp8_to_f32(raw, 5, 2, 15, fpst);
}

typedef struct AMEMfmaccInternal30 {
    bool sign;
    int16_t exp;
    uint32_t sig;
    bool is_zero;
} AMEMfmaccInternal30;

typedef struct AMEMfmaccDecodedFloat {
    bool sign;
    int16_t exp;
    uint16_t sig;
    bool is_zero;
} AMEMfmaccDecodedFloat;

static inline uint32_t xsmtame06v_mfmacc_shrjam32(uint32_t a, uint8_t dist)
{
    if (!dist) {
        return a;
    }
    if (dist < 32) {
        return (a >> dist) | ((uint32_t)(a << ((32 - dist) & 31)) != 0);
    }
    return a ? 1 : 0;
}

static inline uint64_t xsmtame06v_mfmacc_shrjam64(uint64_t a, uint8_t dist)
{
    if (!dist) {
        return a;
    }
    if (dist < 64) {
        return (a >> dist) | ((uint64_t)(a << ((64 - dist) & 63)) != 0);
    }
    return a ? 1 : 0;
}

static inline uint32_t xsmtame06v_mfmacc_round_to_odd32(uint32_t a,
                                                           uint8_t dist)
{
    uint32_t z;

    if (!dist) {
        return a;
    }
    z = xsmtame06v_mfmacc_shrjam32(a, dist);
    if (z && (a & ((((uint32_t)1) << (dist < 32 ? dist : 31)) - 1))) {
        z |= 1;
    }
    return z;
}

static bool xsmtame06v_mfmacc_decode_float(uint16_t ui,
                                             uint8_t exp_bits,
                                             uint8_t frac_bits,
                                             int16_t exp_bias,
                                             AMEMfmaccDecodedFloat *out)
{
    uint16_t exp_mask = (((uint16_t)1) << exp_bits) - 1;
    uint16_t frac_mask = (((uint16_t)1) << frac_bits) - 1;
    uint16_t exp = (ui >> frac_bits) & exp_mask;
    uint16_t frac = ui & frac_mask;
    int16_t shift_dist;

    out->sign = (ui >> (exp_bits + frac_bits)) & 1;
    out->exp = 0;
    out->sig = 0;
    out->is_zero = false;

    if (exp == exp_mask) {
        return false;
    }
    if (!exp) {
        if (!frac) {
            out->is_zero = true;
            return true;
        }
        shift_dist = 0;
        while (frac < (((uint16_t)1) << frac_bits)) {
            frac <<= 1;
            ++shift_dist;
        }
        out->exp = 1 - exp_bias - shift_dist;
        out->sig = frac;
        return true;
    }

    out->exp = (int16_t)exp - exp_bias;
    out->sig = (((uint16_t)1) << frac_bits) | frac;
    return true;
}

static bool xsmtame06v_mfmacc_mul_float_to_internal30(uint16_t ui_a,
                                                        uint8_t exp_bits_a,
                                                        uint8_t frac_bits_a,
                                                        int16_t exp_bias_a,
                                                        uint16_t ui_b,
                                                        uint8_t exp_bits_b,
                                                        uint8_t frac_bits_b,
                                                        int16_t exp_bias_b,
                                                        AMEMfmaccInternal30 *out)
{
    AMEMfmaccDecodedFloat a;
    AMEMfmaccDecodedFloat b;
    uint64_t sig_prod;
    uint8_t frac_bits_prod;

    if (!xsmtame06v_mfmacc_decode_float(ui_a, exp_bits_a, frac_bits_a,
                                          exp_bias_a, &a) ||
        !xsmtame06v_mfmacc_decode_float(ui_b, exp_bits_b, frac_bits_b,
                                          exp_bias_b, &b)) {
        return false;
    }

    out->sign = a.sign ^ b.sign;
    out->exp = 0;
    out->sig = 0;
    out->is_zero = false;

    if (a.is_zero || b.is_zero) {
        out->is_zero = true;
        return true;
    }

    sig_prod = (uint64_t)a.sig * (uint64_t)b.sig;
    out->exp = a.exp + b.exp;
    frac_bits_prod = frac_bits_a + frac_bits_b;

    if (sig_prod & (((uint64_t)1) << (frac_bits_prod + 1))) {
        ++out->exp;
        sig_prod = xsmtame06v_mfmacc_shrjam64(sig_prod, 1);
    }

    if (frac_bits_prod < 26) {
        sig_prod <<= (26 - frac_bits_prod);
    } else if (frac_bits_prod > 26) {
        sig_prod = xsmtame06v_mfmacc_shrjam64(sig_prod,
                                                frac_bits_prod - 26);
    }

    out->sig = (uint32_t)sig_prod;
    return true;
}

static inline bool xsmtame06v_mfmacc_mul_f16_to_internal30(uint16_t ui_a,
                                                              uint16_t ui_b,
                                                              AMEMfmaccInternal30 *out)
{
    return xsmtame06v_mfmacc_mul_float_to_internal30(ui_a, 5, 10, 15,
                                                       ui_b, 5, 10, 15,
                                                       out);
}

static inline bool xsmtame06v_mfmacc_mul_bf16_to_internal30(uint16_t ui_a,
                                                               uint16_t ui_b,
                                                               AMEMfmaccInternal30 *out)
{
    return xsmtame06v_mfmacc_mul_float_to_internal30(ui_a, 8, 7, 127,
                                                       ui_b, 8, 7, 127,
                                                       out);
}

static inline bool xsmtame06v_mfmacc_mul_e4_to_internal30(uint8_t ui_a,
                                                             uint8_t ui_b,
                                                             AMEMfmaccInternal30 *out)
{
    return xsmtame06v_mfmacc_mul_float_to_internal30(ui_a, 4, 3, 7,
                                                       ui_b, 4, 3, 7,
                                                       out);
}

static inline bool xsmtame06v_mfmacc_mul_e5_to_internal30(uint8_t ui_a,
                                                             uint8_t ui_b,
                                                             AMEMfmaccInternal30 *out)
{
    return xsmtame06v_mfmacc_mul_float_to_internal30(ui_a, 5, 2, 15,
                                                       ui_b, 5, 2, 15,
                                                       out);
}

static void xsmtame06v_mfmacc_pack_normalized_internal30(bool sign,
                                                           int16_t exp,
                                                           uint64_t sig,
                                                           AMEMfmaccInternal30 *out)
{
    out->sign = false;
    out->exp = 0;
    out->sig = 0;
    out->is_zero = true;

    if (!sig) {
        return;
    }

    while (sig >= UINT64_C(0x8000000)) {
        sig = xsmtame06v_mfmacc_shrjam64(sig, 1);
        ++exp;
    }
    while (sig < UINT64_C(0x4000000)) {
        sig <<= 1;
        --exp;
    }

    out->sign = sign;
    out->exp = exp;
    out->sig = (uint32_t)sig;
    out->is_zero = false;
}

static void xsmtame06v_mfmacc_add_internal30_unified(const AMEMfmaccInternal30 *terms,
                                                        uint8_t term_count,
                                                        AMEMfmaccInternal30 *out)
{
    bool have_non_zero = false;
    int16_t exp_z = 0;
    int64_t sig_sum = 0;
    uint8_t k;

    for (k = 0; k < term_count; ++k) {
        if (terms[k].is_zero) {
            continue;
        }
        if (!have_non_zero || terms[k].exp > exp_z) {
            exp_z = terms[k].exp;
            have_non_zero = true;
        }
    }

    if (!have_non_zero) {
        out->sign = false;
        out->exp = 0;
        out->sig = 0;
        out->is_zero = true;
        return;
    }

    for (k = 0; k < term_count; ++k) {
        uint32_t aligned_sig;

        if (terms[k].is_zero) {
            continue;
        }
        aligned_sig = terms[k].sig;
        if (terms[k].exp < exp_z) {
            aligned_sig = xsmtame06v_mfmacc_round_to_odd32(aligned_sig,
                                (uint8_t)(exp_z - terms[k].exp));
        }
        sig_sum += terms[k].sign ? -(int64_t)aligned_sig : (int64_t)aligned_sig;
    }

    if (sig_sum < 0) {
        xsmtame06v_mfmacc_pack_normalized_internal30(true, exp_z,
                                                       (uint64_t)(-sig_sum),
                                                       out);
    } else {
        xsmtame06v_mfmacc_pack_normalized_internal30(false, exp_z,
                                                       (uint64_t)sig_sum,
                                                       out);
    }
}

static float32 xsmtame06v_mfmacc_internal30_to_f32(const AMEMfmaccInternal30 *a,
                                                     float_status *fpst)
{
    uint32_t ui_z;
    int16_t exp;
    uint32_t frac;

    if (a->is_zero) {
        ui_z = (((uint32_t)a->sign) << 31);
        return make_float32(ui_z);
    }

    exp = a->exp + 127;
    if (exp <= 0) {
        ui_z = (((uint32_t)a->sign) << 31);
        return make_float32(ui_z);
    }
    if (exp >= 0xFF) {
        float_raise(float_flag_overflow | float_flag_inexact, fpst);
        ui_z = (((uint32_t)a->sign) << 31) | (0xFFu << 23);
        return make_float32(ui_z);
    }

    frac = xsmtame06v_mfmacc_round_to_odd32(a->sig, 3) & 0x007fffff;
    ui_z = (((uint32_t)a->sign) << 31) | ((uint32_t)exp << 23) | frac;
    return make_float32(ui_z);
}

static float32 xsmtame06v_mfmacc_reference_dot16(const uint16_t *lhs,
                                                   const uint16_t *rhs,
                                                   uint8_t k_cols,
                                                   float32 c,
                                                   float32 (*convert)(uint16_t,
                                                                      float_status *),
                                                   float_status *fpst)
{
    uint8_t k;

    for (k = 0; k < k_cols; ++k) {
        c = float32_add(float32_mul(convert(lhs[k], fpst), convert(rhs[k], fpst), fpst),
                        c, fpst);
    }
    return c;
}

static float32 xsmtame06v_mfmacc_reference_dot8(const uint8_t *lhs,
                                                  const uint8_t *rhs,
                                                  uint8_t k_cols,
                                                  float32 c,
                                                  float32 (*convert)(uint8_t,
                                                                     float_status *),
                                                  float_status *fpst)
{
    uint8_t k;

    for (k = 0; k < k_cols; ++k) {
        c = float32_add(float32_mul(convert(lhs[k], fpst), convert(rhs[k], fpst), fpst),
                        c, fpst);
    }
    return c;
}

static float32 xsmtame06v_mfmacc_cell16_internal30(const uint16_t *lhs,
                                                     const uint16_t *rhs,
                                                     uint8_t k_cols,
                                                     float32 c,
                                                     bool (*mul_to_internal)(uint16_t,
                                                                             uint16_t,
                                                                             AMEMfmaccInternal30 *),
                                                     float32 (*fallback_convert)(uint16_t,
                                                                                 float_status *),
                                                     float_status *fpst)
{
    AMEMfmaccInternal30 acc_int;
    AMEMfmaccInternal30 prod_list[4];
    uint8_t k;

    acc_int.sign = false;
    acc_int.exp = 0;
    acc_int.sig = 0;
    acc_int.is_zero = true;

    if (k_cols > 4) {
        return xsmtame06v_mfmacc_reference_dot16(lhs, rhs, k_cols, c,
                                                   fallback_convert, fpst);
    }

    for (k = 0; k < k_cols; ++k) {
        if (!mul_to_internal(lhs[k], rhs[k], &prod_list[k])) {
            return xsmtame06v_mfmacc_reference_dot16(lhs, rhs, k_cols, c,
                                                       fallback_convert, fpst);
        }
    }

    xsmtame06v_mfmacc_add_internal30_unified(prod_list, k_cols, &acc_int);
    return float32_add(xsmtame06v_mfmacc_internal30_to_f32(&acc_int, fpst),
                       c, fpst);
}

static float32 xsmtame06v_mfmacc_cell8_internal30(const uint8_t *lhs,
                                                    const uint8_t *rhs,
                                                    uint8_t k_cols,
                                                    float32 c,
                                                    bool (*mul_to_internal)(uint8_t,
                                                                            uint8_t,
                                                                            AMEMfmaccInternal30 *),
                                                    float32 (*fallback_convert)(uint8_t,
                                                                                float_status *),
                                                    float_status *fpst)
{
    AMEMfmaccInternal30 acc_int;
    AMEMfmaccInternal30 prod_list[4];
    uint8_t k;

    acc_int.sign = false;
    acc_int.exp = 0;
    acc_int.sig = 0;
    acc_int.is_zero = true;

    if (k_cols > 4) {
        return xsmtame06v_mfmacc_reference_dot8(lhs, rhs, k_cols, c,
                                                  fallback_convert, fpst);
    }

    for (k = 0; k < k_cols; ++k) {
        if (!mul_to_internal(lhs[k], rhs[k], &prod_list[k])) {
            return xsmtame06v_mfmacc_reference_dot8(lhs, rhs, k_cols, c,
                                                      fallback_convert, fpst);
        }
    }

    xsmtame06v_mfmacc_add_internal30_unified(prod_list, k_cols, &acc_int);
    return float32_add(xsmtame06v_mfmacc_internal30_to_f32(&acc_int, fpst),
                       c, fpst);
}

static void xsmtame06v_mfmacc16_common(CPURISCVState *env, uint32_t md,
                                         uint32_t ms2, uint32_t ms1,
                                         bool (*mul_to_internal)(uint16_t,
                                                                 uint16_t,
                                                                 AMEMfmaccInternal30 *),
                                         float32 (*fallback_convert)(uint16_t,
                                                                     float_status *))
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tA = xsmtame06v_tile16_ptr(env, ms1);
    const uint16_t *tBT = xsmtame06v_tile16_ptr(env, ms2);
    uint32_t *acc = xsmtame06v_acc32_ptr(env, md);
    float_status *fpst = &env->fp_status;
    uint32_t m, n;

    for (m = 0; m < shape.m; m++) {
        for (n = 0; n < shape.n; n++) {
            float32 c = make_float32(acc[m * shape.n + n]);
            c = xsmtame06v_mfmacc_cell16_internal30(&tA[m * shape.k],
                                                      &tBT[n * shape.k],
                                                      shape.k,
                                                      c,
                                                      mul_to_internal,
                                                      fallback_convert,
                                                      fpst);
            acc[m * shape.n + n] = float32_val(c);
        }
    }
}

static void xsmtame06v_mfmacc8_common(CPURISCVState *env, uint32_t md,
                                        uint32_t ms2, uint32_t ms1,
                                        bool (*mul_to_internal)(uint8_t,
                                                                uint8_t,
                                                                AMEMfmaccInternal30 *),
                                        float32 (*fallback_convert)(uint8_t,
                                                                    float_status *))
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tA = (const uint8_t *)xsmtame06v_tile_ptr(env, ms1);
    const uint8_t *tBT = (const uint8_t *)xsmtame06v_tile_ptr(env, ms2);
    uint32_t *acc = xsmtame06v_acc32_ptr(env, md);
    float_status *fpst = &env->fp_status;
    uint32_t m, n;

    for (m = 0; m < shape.m; m++) {
        for (n = 0; n < shape.n; n++) {
            float32 c = make_float32(acc[m * shape.n + n]);
            c = xsmtame06v_mfmacc_cell8_internal30(&tA[m * shape.k],
                                                     &tBT[n * shape.k],
                                                     shape.k,
                                                     c,
                                                     mul_to_internal,
                                                     fallback_convert,
                                                     fpst);
            acc[m * shape.n + n] = float32_val(c);
        }
    }
}

static void xsmtame06v_mfmacc16_acc16_common(CPURISCVState *env, uint32_t md,
                                               uint32_t ms2, uint32_t ms1,
                                               bool (*mul_to_internal)(uint16_t,
                                                                       uint16_t,
                                                                       AMEMfmaccInternal30 *),
                                               float32 (*acc_to_f32)(uint16_t,
                                                                     float_status *),
                                               uint16_t (*f32_to_acc)(float32,
                                                                      float_status *))
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint16_t *tA = xsmtame06v_tile16_ptr(env, ms1);
    const uint16_t *tBT = xsmtame06v_tile16_ptr(env, ms2);
    uint16_t *acc = xsmtame06v_acc16_ptr(env, md);
    float_status *fpst = &env->fp_status;
    uint32_t m, n;

    for (m = 0; m < shape.m; m++) {
        for (n = 0; n < shape.n; n++) {
            float32 c = acc_to_f32(acc[m * shape.n + n], fpst);
            c = xsmtame06v_mfmacc_cell16_internal30(&tA[m * shape.k],
                                                      &tBT[n * shape.k],
                                                      shape.k,
                                                      c,
                                                      mul_to_internal,
                                                      xsmtame06v_mfmacc_fp16_to_f32,
                                                      fpst);
            acc[m * shape.n + n] = f32_to_acc(c, fpst);
        }
    }
}

static void xsmtame06v_mfmacc8_acc16_common(CPURISCVState *env, uint32_t md,
                                              uint32_t ms2, uint32_t ms1,
                                              bool (*mul_to_internal)(uint8_t,
                                                                      uint8_t,
                                                                      AMEMfmaccInternal30 *),
                                              float32 (*acc_to_f32)(uint16_t,
                                                                    float_status *),
                                              uint16_t (*f32_to_acc)(float32,
                                                                     float_status *),
                                              float32 (*fallback_convert)(uint8_t,
                                                                          float_status *))
{
    AMEShapeInfo shape = xsmtame06v_shape(env);
    const uint8_t *tA = (const uint8_t *)xsmtame06v_tile_ptr(env, ms1);
    const uint8_t *tBT = (const uint8_t *)xsmtame06v_tile_ptr(env, ms2);
    uint16_t *acc = xsmtame06v_acc16_ptr(env, md);
    float_status *fpst = &env->fp_status;
    uint32_t m, n;

    for (m = 0; m < shape.m; m++) {
        for (n = 0; n < shape.n; n++) {
            float32 c = acc_to_f32(acc[m * shape.n + n], fpst);
            c = xsmtame06v_mfmacc_cell8_internal30(&tA[m * shape.k],
                                                     &tBT[n * shape.k],
                                                     shape.k,
                                                     c,
                                                     mul_to_internal,
                                                     fallback_convert,
                                                     fpst);
            acc[m * shape.n + n] = f32_to_acc(c, fpst);
        }
    }
}

void HELPER(xsmtame06v_mfmacc_h_e5)(CPURISCVState *env, uint32_t md,
                             uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc8_acc16_common(env, md, ms2, ms1,
                                      xsmtame06v_mfmacc_mul_e5_to_internal30,
                                      xsmtame06v_mfmacc_fp16_to_f32,
                                      xsmtame06v_mfmacc_f32_to_f16_bits,
                                      xsmtame06v_mfmacc_e5_to_f32);
}

void HELPER(xsmtame06v_mfmacc_h_e4)(CPURISCVState *env, uint32_t md,
                             uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc8_acc16_common(env, md, ms2, ms1,
                                      xsmtame06v_mfmacc_mul_e4_to_internal30,
                                      xsmtame06v_mfmacc_fp16_to_f32,
                                      xsmtame06v_mfmacc_f32_to_f16_bits,
                                      xsmtame06v_mfmacc_e4_to_f32);
}

void HELPER(xsmtame06v_mfmacc_bf16_e5)(CPURISCVState *env, uint32_t md,
                                uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc8_acc16_common(env, md, ms2, ms1,
                                      xsmtame06v_mfmacc_mul_e5_to_internal30,
                                      xsmtame06v_mfmacc_bf16_to_f32,
                                      xsmtame06v_mfmacc_f32_to_bf16_bits,
                                      xsmtame06v_mfmacc_e5_to_f32);
}

void HELPER(xsmtame06v_mfmacc_bf16_e4)(CPURISCVState *env, uint32_t md,
                                uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc8_acc16_common(env, md, ms2, ms1,
                                      xsmtame06v_mfmacc_mul_e4_to_internal30,
                                      xsmtame06v_mfmacc_bf16_to_f32,
                                      xsmtame06v_mfmacc_f32_to_bf16_bits,
                                      xsmtame06v_mfmacc_e4_to_f32);
}

void HELPER(xsmtame06v_mfmacc_h)(CPURISCVState *env, uint32_t md,
                          uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc16_acc16_common(env, md, ms2, ms1,
                                       xsmtame06v_mfmacc_mul_f16_to_internal30,
                                       xsmtame06v_mfmacc_fp16_to_f32,
                                       xsmtame06v_mfmacc_f32_to_f16_bits);
}

void HELPER(xsmtame06v_mfmacc_s_e5)(CPURISCVState *env, uint32_t md,
                             uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc8_common(env, md, ms2, ms1,
                                xsmtame06v_mfmacc_mul_e5_to_internal30,
                                xsmtame06v_mfmacc_e5_to_f32);
}

void HELPER(xsmtame06v_mfmacc_s_e4)(CPURISCVState *env, uint32_t md,
                             uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc8_common(env, md, ms2, ms1,
                                xsmtame06v_mfmacc_mul_e4_to_internal30,
                                xsmtame06v_mfmacc_e4_to_f32);
}

void HELPER(xsmtame06v_mfmacc_s_h)(CPURISCVState *env, uint32_t md,
                            uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc16_common(env, md, ms2, ms1,
                                 xsmtame06v_mfmacc_mul_f16_to_internal30,
                                 xsmtame06v_mfmacc_fp16_to_f32);
}

void HELPER(xsmtame06v_mfmacc_s_bf16)(CPURISCVState *env, uint32_t md,
                               uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mfmacc16_common(env, md, ms2, ms1,
                                 xsmtame06v_mfmacc_mul_bf16_to_internal30,
                                 xsmtame06v_mfmacc_bf16_to_f32);
}
/*
 * ──────────────────────────────────────────
 *  MISC helpers: mmov
 * ──────────────────────────────────────────
 */

void HELPER(xsmtame06v_mmov_mm)(CPURISCVState *env, uint32_t md,
                                  uint32_t ms1)
{
    size_t dst_size;
    size_t src_size;
    uint8_t *dst = xsmtame06v_matrix_ptr(env, md, &dst_size);
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms1, &src_size);
    size_t copy_size = MIN(dst_size, src_size);

    if (dst == src) {
        return;
    }

    memmove(dst, src, copy_size);
}

target_ulong HELPER(xsmtame06v_mmovb_x_m)(CPURISCVState *env, uint32_t ms2,
                                            target_ulong idx)
{
    size_t reg_size;
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms2, &reg_size);
    size_t offset = xsmtame06v_mmov_elem_offset(reg_size, 1, idx);

    return src[offset];
}

target_ulong HELPER(xsmtame06v_mmovh_x_m)(CPURISCVState *env, uint32_t ms2,
                                            target_ulong idx)
{
    size_t reg_size;
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms2, &reg_size);
    size_t offset = xsmtame06v_mmov_elem_offset(reg_size, 2, idx);

    return lduw_le_p(src + offset);
}

target_ulong HELPER(xsmtame06v_mmovw_x_m)(CPURISCVState *env, uint32_t ms2,
                                            target_ulong idx)
{
    size_t reg_size;
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms2, &reg_size);
    size_t offset = xsmtame06v_mmov_elem_offset(reg_size, 4, idx);

    return ldl_le_p(src + offset);
}

target_ulong HELPER(xsmtame06v_mmovd_x_m)(CPURISCVState *env, uint32_t ms2,
                                            target_ulong idx)
{
    size_t reg_size;
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms2, &reg_size);
    size_t offset = xsmtame06v_mmov_elem_offset(reg_size, 8, idx);

    return ldq_le_p(src + offset);
}

void HELPER(xsmtame06v_mmovb_m_x)(CPURISCVState *env, uint32_t md,
                                    target_ulong idx,
                                    target_ulong value)
{
    xsmtame06v_mmov_m_x_common(env, md, idx, value, 1);
}

void HELPER(xsmtame06v_mmovh_m_x)(CPURISCVState *env, uint32_t md,
                                    target_ulong idx,
                                    target_ulong value)
{
    xsmtame06v_mmov_m_x_common(env, md, idx, value, 2);
}

void HELPER(xsmtame06v_mmovw_m_x)(CPURISCVState *env, uint32_t md,
                                    target_ulong idx,
                                    target_ulong value)
{
    xsmtame06v_mmov_m_x_common(env, md, idx, value, 4);
}

void HELPER(xsmtame06v_mmovd_m_x)(CPURISCVState *env, uint32_t md,
                                    target_ulong idx,
                                    target_ulong value)
{
    xsmtame06v_mmov_m_x_common(env, md, idx, value, 8);
}

static void xsmtame06v_mpack_common(CPURISCVState *env, uint32_t md,
                                      uint32_t ms2, uint32_t ms1,
                                      bool high1, bool high2)
{
    size_t reg_size;
    uint8_t tmp[AME_ACC_LEN_B];
    uint8_t *dst = xsmtame06v_matrix_ptr(env, md, &reg_size);
    uint8_t *src2 = xsmtame06v_matrix_ptr(env, ms2, NULL);
    uint8_t *src1 = xsmtame06v_matrix_ptr(env, ms1, NULL);
    size_t row_bytes = xsmtame06v_matrix_row_bytes(env, md);
    size_t half = row_bytes / 2;
    size_t rows = reg_size / row_bytes;
    size_t row;

    g_assert(row_bytes % 2 == 0);

    for (row = 0; row < rows; row++) {
        size_t off = row * row_bytes;

        memcpy(tmp + off,
               src1 + off + (high1 ? half : 0),
               half);
        memcpy(tmp + off + half,
               src2 + off + (high2 ? half : 0),
               half);
    }

    memcpy(dst, tmp, reg_size);
}

static void xsmtame06v_mrslide_common(CPURISCVState *env, uint32_t md,
                                        uint32_t ms1, uint32_t amount,
                                        bool up)
{
    size_t reg_size;
    uint8_t tmp[AME_ACC_LEN_B];
    uint8_t *dst = xsmtame06v_matrix_ptr(env, md, &reg_size);
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms1, NULL);
    size_t row_bytes = xsmtame06v_matrix_row_bytes(env, md);
    size_t rows = reg_size / row_bytes;
    size_t row;

    memset(tmp, 0, reg_size);
    for (row = 0; row < rows; row++) {
        size_t src_row;

        if (up) {
            src_row = row + amount;
            if (src_row >= rows) {
                continue;
            }
        } else {
            if (row < amount) {
                continue;
            }
            src_row = row - amount;
        }

        memcpy(tmp + row * row_bytes, src + src_row * row_bytes, row_bytes);
    }

    memcpy(dst, tmp, reg_size);
}

static void xsmtame06v_mcslide_common(CPURISCVState *env, uint32_t md,
                                        uint32_t ms1, uint32_t amount,
                                        size_t elem_size, bool up)
{
    size_t reg_size;
    uint8_t tmp[AME_ACC_LEN_B];
    uint8_t *dst = xsmtame06v_matrix_ptr(env, md, &reg_size);
    uint8_t *src = xsmtame06v_matrix_ptr(env, ms1, NULL);
    size_t row_bytes = xsmtame06v_matrix_row_bytes(env, md);
    size_t rows = reg_size / row_bytes;
    size_t cols = xsmtame06v_matrix_col_count(env, md, elem_size);
    size_t row;
    size_t col;

    memset(tmp, 0, reg_size);
    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            size_t src_col;

            if (up) {
                src_col = col + amount;
                if (src_col >= cols) {
                    continue;
                }
            } else {
                if (col < amount) {
                    continue;
                }
                src_col = col - amount;
            }

            memcpy(tmp + row * row_bytes + col * elem_size,
                   src + row * row_bytes + src_col * elem_size,
                   elem_size);
        }
    }

    memcpy(dst, tmp, reg_size);
}

void HELPER(xsmtame06v_mpack_mm)(CPURISCVState *env, uint32_t md,
                                   uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mpack_common(env, md, ms2, ms1, false, false);
}

void HELPER(xsmtame06v_mpackhl_mm)(CPURISCVState *env, uint32_t md,
                                     uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mpack_common(env, md, ms2, ms1, true, false);
}

void HELPER(xsmtame06v_mpackhh_mm)(CPURISCVState *env, uint32_t md,
                                     uint32_t ms2, uint32_t ms1)
{
    xsmtame06v_mpack_common(env, md, ms2, ms1, true, true);
}

void HELPER(xsmtame06v_mrslidedown)(CPURISCVState *env, uint32_t md,
                                      uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mrslide_common(env, md, ms1, amount, false);
}

void HELPER(xsmtame06v_mrslideup)(CPURISCVState *env, uint32_t md,
                                    uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mrslide_common(env, md, ms1, amount, true);
}

void HELPER(xsmtame06v_mcslidedown_b)(CPURISCVState *env, uint32_t md,
                                        uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mcslide_common(env, md, ms1, amount, 1, false);
}

void HELPER(xsmtame06v_mcslidedown_h)(CPURISCVState *env, uint32_t md,
                                        uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mcslide_common(env, md, ms1, amount, 2, false);
}

void HELPER(xsmtame06v_mcslidedown_w)(CPURISCVState *env, uint32_t md,
                                        uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mcslide_common(env, md, ms1, amount, 4, false);
}

void HELPER(xsmtame06v_mcslideup_b)(CPURISCVState *env, uint32_t md,
                                      uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mcslide_common(env, md, ms1, amount, 1, true);
}

void HELPER(xsmtame06v_mcslideup_h)(CPURISCVState *env, uint32_t md,
                                      uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mcslide_common(env, md, ms1, amount, 2, true);
}

void HELPER(xsmtame06v_mcslideup_w)(CPURISCVState *env, uint32_t md,
                                      uint32_t ms1, uint32_t amount)
{
    xsmtame06v_mcslide_common(env, md, ms1, amount, 4, true);
}

/*
 * ──────────────────────────────────────────
 *  Control helpers
 * ──────────────────────────────────────────
 */

/* mzero{,2,4,8}r : zero count unified matrix register slots starting at md */
void HELPER(xsmtame06v_mzero)(CPURISCVState *env, uint32_t md,
                                uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        uint32_t reg = md + i;

        if (reg < AME_NR_TILES) {
            memset((uint8_t *)env->ame_tile + reg * ame_env_tlenb(env), 0,
                   ame_env_tlenb(env));
        } else if (reg < AME_NR_TILES + AME_NR_ACCS) {
            memset((uint8_t *)env->ame_acc +
                   (reg - AME_NR_TILES) * ame_env_acc_len_b(env), 0,
                   ame_env_acc_len_b(env));
        }
    }
}

/* mrelease : set mstatus.MS → 01 (Initial) */
void HELPER(xsmtame06v_mrelease)(CPURISCVState *env)
{
#ifndef CONFIG_USER_ONLY
    /*
     * Clear MS bits [26:25], then set to 01 (Initial).
     * MSTATUS_MS = 0x06000000 covers both bits.
     */
    env->mstatus = (env->mstatus & ~MSTATUS_MS)
                   | (1ULL << 25);   /* 01 in bits [26:25] */
#endif
}
