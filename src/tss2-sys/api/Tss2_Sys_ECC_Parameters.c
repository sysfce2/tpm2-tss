/* SPDX-License-Identifier: BSD-2-Clause */
/***********************************************************************;
 * Copyright (c) 2015 - 2017, Intel Corporation
 * All rights reserved.
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h" // IWYU pragma: keep
#endif

#include "sysapi_util.h"      // for _TSS2_SYS_CONTEXT_BLOB, syscontext_cast
#include "tss2_common.h"      // for TSS2_RC, TSS2_SYS_RC_BAD_REFERENCE
#include "tss2_mu.h"          // for Tss2_MU_TPMS_ALGORITHM_DETAIL_ECC_Unmar...
#include "tss2_sys.h"         // for TSS2_SYS_CONTEXT, TSS2L_SYS_AUTH_COMMAND
#include "tss2_tpm2_types.h"  // for TPMI_ECC_CURVE, TPMS_ALGORITHM_DETAIL_ECC

TSS2_RC Tss2_Sys_ECC_Parameters_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_ECC_CURVE curveID)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(ctx, TPM2_CC_ECC_Parameters);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT16_Marshal(curveID, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    ctx->decryptAllowed = 0;
    ctx->encryptAllowed = 0;
    ctx->authAllowed = 1;

    return CommonPrepareEpilogue(ctx);
}

TSS2_RC Tss2_Sys_ECC_Parameters_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    TPMS_ALGORITHM_DETAIL_ECC *parameters)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(ctx);
    if (rval)
        return rval;

    return Tss2_MU_TPMS_ALGORITHM_DETAIL_ECC_Unmarshal(ctx->cmdBuffer,
                                                       ctx->maxCmdSize,
                                                       &ctx->nextData,
                                                       parameters);
}

TSS2_RC Tss2_Sys_ECC_Parameters(
    TSS2_SYS_CONTEXT *sysContext,
    TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
    TPMI_ECC_CURVE curveID,
    TPMS_ALGORITHM_DETAIL_ECC *parameters,
    TSS2L_SYS_AUTH_RESPONSE *rspAuthsArray)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    rval = Tss2_Sys_ECC_Parameters_Prepare(sysContext, curveID);
    if (rval)
        return rval;

    rval = CommonOneCall(ctx, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_ECC_Parameters_Complete(sysContext, parameters);
}
