/* AME (Attached Matrix Extension) helper declarations */

/* --- Memory: stride load --- */
/* mlme8  td, (rs1), rs2 : load tile INT8 */
DEF_HELPER_4(xsmtamev06_mlme8,  void, env, i32, tl, tl)
/* mlae8 td, (rs1), rs2 : strided load A tile INT8 */
DEF_HELPER_4(xsmtamev06_mlae8,  void, env, i32, tl, tl)
/* mlbe8 td, (rs1), rs2 : strided load B tile INT8 */
DEF_HELPER_4(xsmtamev06_mlbe8,  void, env, i32, tl, tl)
/* mlate8 td, (rs1), rs2 : strided transpose-load A tile INT8 */
DEF_HELPER_4(xsmtamev06_mlate8, void, env, i32, tl, tl)
/* mlbte8 td, (rs1), rs2 : strided transpose-load B tile INT8 */
DEF_HELPER_4(xsmtamev06_mlbte8, void, env, i32, tl, tl)
/* mlae32 td, (rs1), rs2 : strided load A tile INT32 */
DEF_HELPER_4(xsmtamev06_mlae32, void, env, i32, tl, tl)
/* mlbe32 td, (rs1), rs2 : strided load B tile INT32 */
DEF_HELPER_4(xsmtamev06_mlbe32, void, env, i32, tl, tl)
/* mlate32 td, (rs1), rs2 : strided transpose-load A tile INT32 */
DEF_HELPER_4(xsmtamev06_mlate32, void, env, i32, tl, tl)
/* mlbte32 td, (rs1), rs2 : strided transpose-load B tile INT32 */
DEF_HELPER_4(xsmtamev06_mlbte32, void, env, i32, tl, tl)
/* mlce8 ad, (rs1), rs2 : strided load C matrix INT8 into acc */
DEF_HELPER_4(xsmtamev06_mlce8,  void, env, i32, tl, tl)
/* mlce16 ad, (rs1), rs2 : strided load C matrix INT16 into acc */
DEF_HELPER_4(xsmtamev06_mlce16, void, env, i32, tl, tl)
/* mlce32 ad, (rs1), rs2 : strided load C matrix INT32/FP32 into acc */
DEF_HELPER_4(xsmtamev06_mlce32, void, env, i32, tl, tl)
/* mlcte8 ad, (rs1), rs2 : strided transpose-load C matrix INT8 into acc */
DEF_HELPER_4(xsmtamev06_mlcte8,  void, env, i32, tl, tl)
/* mlcte16 ad, (rs1), rs2 : strided transpose-load C matrix INT16 into acc */
DEF_HELPER_4(xsmtamev06_mlcte16, void, env, i32, tl, tl)
/* mlcte32 ad, (rs1), rs2 : strided transpose-load C matrix INT32/FP32 into acc */
DEF_HELPER_4(xsmtamev06_mlcte32, void, env, i32, tl, tl)
/* mlme16 md, (rs1) : contiguous load tile INT16/FP16 */
DEF_HELPER_3(xsmtamev06_mlme16, void, env, i32, tl)
/* mlae16 md, (rs1), rs2 : strided load A tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_mlae16, void, env, i32, tl, tl)
/* mlbe16 md, (rs1), rs2 : strided load B tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_mlbe16, void, env, i32, tl, tl)
/* mlate16 md, (rs1), rs2 : strided load A tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_mlate16, void, env, i32, tl, tl)
/* mlbte16 md, (rs1), rs2 : strided load B_T tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_mlbte16, void, env, i32, tl, tl)
/* mlme32 md, (rs1) : contiguous load acc INT32/FP32 */
DEF_HELPER_3(xsmtamev06_mlme32, void, env, i32, tl)

/* --- Memory: stride store --- */
/* msme8  td, (rs1), rs2 : store tile INT8 */
DEF_HELPER_4(xsmtamev06_msme8,  void, env, i32, tl, tl)
/* msae8 td, (rs1), rs2 : strided store A tile INT8 */
DEF_HELPER_4(xsmtamev06_msae8,  void, env, i32, tl, tl)
/* msbe8 td, (rs1), rs2 : strided store B tile INT8 */
DEF_HELPER_4(xsmtamev06_msbe8,  void, env, i32, tl, tl)
/* msate8 td, (rs1), rs2 : strided transpose-store A tile INT8 */
DEF_HELPER_4(xsmtamev06_msate8, void, env, i32, tl, tl)
/* msbte8 td, (rs1), rs2 : strided transpose-store B tile INT8 */
DEF_HELPER_4(xsmtamev06_msbte8, void, env, i32, tl, tl)
/* msae32 td, (rs1), rs2 : strided store A tile INT32 */
DEF_HELPER_4(xsmtamev06_msae32, void, env, i32, tl, tl)
/* msbe32 td, (rs1), rs2 : strided store B tile INT32 */
DEF_HELPER_4(xsmtamev06_msbe32, void, env, i32, tl, tl)
/* msate32 td, (rs1), rs2 : strided transpose-store A tile INT32 */
DEF_HELPER_4(xsmtamev06_msate32, void, env, i32, tl, tl)
/* msbte32 td, (rs1), rs2 : strided transpose-store B tile INT32 */
DEF_HELPER_4(xsmtamev06_msbte32, void, env, i32, tl, tl)
/* msce8 ad, (rs1), rs2 : strided store C matrix INT8 from acc */
DEF_HELPER_4(xsmtamev06_msce8,  void, env, i32, tl, tl)
/* msce16 ad, (rs1), rs2 : strided store C matrix INT16 from acc */
DEF_HELPER_4(xsmtamev06_msce16, void, env, i32, tl, tl)
/* msce32 ad, (rs1), rs2 : strided store C matrix INT32/FP32 from acc */
DEF_HELPER_4(xsmtamev06_msce32, void, env, i32, tl, tl)
/* mscte8 ad, (rs1), rs2 : strided transpose-store C matrix INT8 from acc */
DEF_HELPER_4(xsmtamev06_mscte8,  void, env, i32, tl, tl)
/* mscte16 ad, (rs1), rs2 : strided transpose-store C matrix INT16 from acc */
DEF_HELPER_4(xsmtamev06_mscte16, void, env, i32, tl, tl)
/* mscte32 ad, (rs1), rs2 : strided transpose-store C matrix INT32/FP32 from acc */
DEF_HELPER_4(xsmtamev06_mscte32, void, env, i32, tl, tl)
/* msae16 td, (rs1), rs2 : strided store A tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_msae16, void, env, i32, tl, tl)
/* msbe16 td, (rs1), rs2 : strided store B tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_msbe16, void, env, i32, tl, tl)
/* msate16 td, (rs1), rs2 : strided transpose-store A tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_msate16, void, env, i32, tl, tl)
/* msbte16 td, (rs1), rs2 : strided transpose-store B tile INT16/FP16 */
DEF_HELPER_4(xsmtamev06_msbte16, void, env, i32, tl, tl)
/* msme16 td, (rs1) : contiguous store tile INT16/FP16 */
DEF_HELPER_3(xsmtamev06_msme16, void, env, i32, tl)
/* msme32 md, (rs1) : contiguous store acc INT32/FP32 */
DEF_HELPER_3(xsmtamev06_msme32, void, env, i32, tl)

/* --- Compute: GEMM --- */
/* mmaccu.w.b  ad, ts2, ts1 : UINT8 GEMM, acc[ad] += tile[ts2] * tile[ts1] */
DEF_HELPER_4(xsmtamev06_mmaccu_w_b,  void, env, i32, i32, i32)
/* mmaccus.w.b  ad, ts2, ts1 : UINT8xINT8 GEMM, acc[ad] += tile[ts2] * tile[ts1] */
DEF_HELPER_4(xsmtamev06_mmaccus_w_b, void, env, i32, i32, i32)
/* mmaccsu.w.b  ad, ts2, ts1 : INT8xUINT8 GEMM, acc[ad] += tile[ts2] * tile[ts1] */
DEF_HELPER_4(xsmtamev06_mmaccsu_w_b, void, env, i32, i32, i32)
/* mmacc.w.b  ad, ts2, ts1 : INT8 GEMM, acc[ad] += tile[ts2] * tile[ts1] */
DEF_HELPER_4(xsmtamev06_mmacc_w_b,  void, env, i32, i32, i32)
/* mfmacc.h.e5 md, ms2, ms1 : FP8 E5M2 GEMM, acc16[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_h_e5, void, env, i32, i32, i32)
/* mfmacc.h.e4 md, ms2, ms1 : FP8 E4M3 GEMM, acc16[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_h_e4, void, env, i32, i32, i32)
/* mfmacc.bf16.e5 md, ms2, ms1 : FP8 E5M2 GEMM, accbf16[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_bf16_e5, void, env, i32, i32, i32)
/* mfmacc.bf16.e4 md, ms2, ms1 : FP8 E4M3 GEMM, accbf16[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_bf16_e4, void, env, i32, i32, i32)
/* mfmacc.h md, ms2, ms1 : FP16 GEMM, acc16[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_h, void, env, i32, i32, i32)
/* mfmacc.s.e5 md, ms2, ms1 : FP8 E5M2 GEMM, acc[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_s_e5, void, env, i32, i32, i32)
/* mfmacc.s.e4 md, ms2, ms1 : FP8 E4M3 GEMM, acc[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_s_e4, void, env, i32, i32, i32)
/* mfmacc.s.h md, ms2, ms1 : FP16 GEMM, acc[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_s_h, void, env, i32, i32, i32)
/* mfmacc.s.bf16 md, ms2, ms1 : BF16 GEMM, acc[md] += A(ms1) * B_T(ms2) */
DEF_HELPER_4(xsmtamev06_mfmacc_s_bf16, void, env, i32, i32, i32)

/* --- MISC: data move --- */
/* mmov.mm md, ms1 : copy matrix register ms1 into md */
DEF_HELPER_3(xsmtamev06_mmov_mm, void, env, i32, i32)
/* mmov{b,h,w,d}.x.m rd, ms2, rs1 : move one matrix element to GPR */
DEF_HELPER_3(xsmtamev06_mmovb_x_m, tl, env, i32, tl)
DEF_HELPER_3(xsmtamev06_mmovh_x_m, tl, env, i32, tl)
DEF_HELPER_3(xsmtamev06_mmovw_x_m, tl, env, i32, tl)
DEF_HELPER_3(xsmtamev06_mmovd_x_m, tl, env, i32, tl)
/* mmov{b,h,w,d}.m.x md, rs1, rs2 : move one GPR element into matrix */
DEF_HELPER_4(xsmtamev06_mmovb_m_x, void, env, i32, tl, tl)
DEF_HELPER_4(xsmtamev06_mmovh_m_x, void, env, i32, tl, tl)
DEF_HELPER_4(xsmtamev06_mmovw_m_x, void, env, i32, tl, tl)
DEF_HELPER_4(xsmtamev06_mmovd_m_x, void, env, i32, tl, tl)
/* mpack*.mm md, ms2, ms1 : pack low/high half-columns from same-class sources */
DEF_HELPER_4(xsmtamev06_mpack_mm, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mpackhl_mm, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mpackhh_mm, void, env, i32, i32, i32)
/* mrslide{down,up} md, ms1, uimm3 : slide rows with zero fill */
DEF_HELPER_4(xsmtamev06_mrslidedown, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mrslideup, void, env, i32, i32, i32)
/* mcslide{down,up}.{b,h,w} md, ms1, uimm3 : slide columns with zero fill */
DEF_HELPER_4(xsmtamev06_mcslidedown_b, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mcslidedown_h, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mcslidedown_w, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mcslideup_b, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mcslideup_h, void, env, i32, i32, i32)
DEF_HELPER_4(xsmtamev06_mcslideup_w, void, env, i32, i32, i32)

/* --- Control --- */
/* mzero{,2,4,8}r : zero count matrix registers starting at md */
DEF_HELPER_3(xsmtamev06_mzero, void, env, i32, i32)
/* mrelease : set mstatus.MS = Initial (01) */
DEF_HELPER_1(xsmtamev06_mrelease, void, env)
