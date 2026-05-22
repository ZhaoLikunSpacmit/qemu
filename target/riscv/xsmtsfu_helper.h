/* XSmtsfu helper declarations */

/* --- SFU: fp32 tile elementwise operations --- */
/* mfex2.s md, ms2 : md[i] = 2 ** ms2[i] */
DEF_HELPER_3(xsmtsfu_mfex2_s, void, env, i32, i32)
/* mftanh.s md, ms2 : md[i] = tanh(ms2[i]) */
DEF_HELPER_3(xsmtsfu_mftanh_s, void, env, i32, i32)
/* mflg2.s md, ms2 : md[i] = log2(ms2[i]) */
DEF_HELPER_3(xsmtsfu_mflg2_s, void, env, i32, i32)
/* mfrcp.s md, ms2 : md[i] = 1 / ms2[i] */
DEF_HELPER_3(xsmtsfu_mfrcp_s, void, env, i32, i32)
