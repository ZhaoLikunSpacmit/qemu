/* XSmtsfu helper declarations */

/* --- SFU: fp32 tile elementwise operations --- */
/* vfex2.v vd, vs2 : vd[i] = 2 ** vs2[i] */
DEF_HELPER_3(xsmtsfu_vfex2_v, void, env, i32, i32)
/* vftanh.v vd, vs2 : vd[i] = tanh(vs2[i]) */
DEF_HELPER_3(xsmtsfu_vftanh_v, void, env, i32, i32)
/* vflg2.v vd, vs2 : vd[i] = log2(vs2[i]) */
DEF_HELPER_3(xsmtsfu_vflg2_v, void, env, i32, i32)
/* vfrcp.v vd, vs2 : vd[i] = 1 / vs2[i] */
DEF_HELPER_3(xsmtsfu_vfrcp_v, void, env, i32, i32)
