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
#include "tss2_mu.h"          // for Tss2_MU_UINT32_Marshal, Tss2_MU_UINT8_M...
#include "tss2_sys.h"         // for TSS2_SYS_CONTEXT, TSS2L_SYS_AUTH_COMMAND
#include "tss2_tpm2_types.h"  // for TPM2_CLOCK_ADJUST, TPMI_RH_PROVISION

TSS2_RC Tss2_Sys_ClockRateAdjust_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_RH_PROVISION auth,
    TPM2_CLOCK_ADJUST rateAdjust)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(ctx, TPM2_CC_ClockRateAdjust);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(auth, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;
    rval = Tss2_MU_UINT8_Marshal(rateAdjust, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    ctx->decryptAllowed = 0;
    ctx->encryptAllowed = 0;
    ctx->authAllowed = 1;

    return CommonPrepareEpilogue(ctx);
}

TSS2_RC Tss2_Sys_ClockRateAdjust_Complete (
    TSS2_SYS_CONTEXT *sysContext)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    return CommonComplete(ctx);
}

TSS2_RC Tss2_Sys_ClockRateAdjust(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_RH_PROVISION auth,
    TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
    TPM2_CLOCK_ADJUST rateAdjust,
    TSS2L_SYS_AUTH_RESPONSE *rspAuthsArray)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    rval = Tss2_Sys_ClockRateAdjust_Prepare(sysContext, auth, rateAdjust);
    if (rval)
        return rval;

    rval = CommonOneCall(ctx, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_ClockRateAdjust_Complete(sysContext);
}
