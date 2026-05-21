/*
 * XSmtsfu translation functions
 *
 * Copyright (c) 2026
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

static bool trans_xsmtsfu_unary_common(DisasContext *s, arg_xsmtsfu_unary *a,
                                       void (*gen_helper)(TCGv_env, TCGv_i32,
                                                          TCGv_i32))
{
    if (!require_xsmtame(s)) {
        return false;
    }
    if (!ame_require(ame_check_tile_reg_2(a->xsmtsfu_vd5) &&
                     ame_check_tile_reg_2(a->xsmtsfu_vs2_5), s)) {
        return false;
    }
    gen_helper(tcg_env, tcg_constant_i32(a->xsmtsfu_vd5),
               tcg_constant_i32(a->xsmtsfu_vs2_5));
    mark_ms_dirty(s);
    return true;
}

static bool trans_vfex2_v(DisasContext *s, arg_xsmtsfu_unary *a)
{
    return trans_xsmtsfu_unary_common(s, a, gen_helper_xsmtsfu_vfex2_v);
}

static bool trans_vftanh_v(DisasContext *s, arg_xsmtsfu_unary *a)
{
    return trans_xsmtsfu_unary_common(s, a, gen_helper_xsmtsfu_vftanh_v);
}

static bool trans_vflg2_v(DisasContext *s, arg_xsmtsfu_unary *a)
{
    return trans_xsmtsfu_unary_common(s, a, gen_helper_xsmtsfu_vflg2_v);
}

static bool trans_vfrcp_v(DisasContext *s, arg_xsmtsfu_unary *a)
{
    return trans_xsmtsfu_unary_common(s, a, gen_helper_xsmtsfu_vfrcp_v);
}
