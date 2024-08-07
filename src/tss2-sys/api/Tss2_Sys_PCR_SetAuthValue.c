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
#include "tss2_mu.h"          // for Tss2_MU_TPM2B_DIGEST_Marshal, Tss2_MU_U...
#include "tss2_sys.h"         // for TSS2_SYS_CONTEXT, TSS2L_SYS_AUTH_COMMAND
#include "tss2_tpm2_types.h"  // for TPM2B_DIGEST, TPMI_DH_PCR, TPM2_CC_PCR_...

TSS2_RC Tss2_Sys_PCR_SetAuthValue_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_PCR pcrHandle,
    const TPM2B_DIGEST *auth)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(ctx, TPM2_CC_PCR_SetAuthValue);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(pcrHandle, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    if (!auth) {
        rval = Tss2_MU_UINT16_Marshal(0, ctx->cmdBuffer,
                                      ctx->maxCmdSize,
                                      &ctx->nextData);

    } else {

        rval = Tss2_MU_TPM2B_DIGEST_Marshal(auth, ctx->cmdBuffer,
                                            ctx->maxCmdSize,
                                            &ctx->nextData);
    }

    if (rval)
        return rval;

    ctx->decryptAllowed = 1;
    ctx->encryptAllowed = 0;
    ctx->authAllowed = 1;

    return CommonPrepareEpilogue(ctx);
}

TSS2_RC Tss2_Sys_PCR_SetAuthValue_Complete (
    TSS2_SYS_CONTEXT *sysContext)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    return CommonComplete(ctx);
}

TSS2_RC Tss2_Sys_PCR_SetAuthValue(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_PCR pcrHandle,
    TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
    const TPM2B_DIGEST *auth,
    TSS2L_SYS_AUTH_RESPONSE *rspAuthsArray)
{
    TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    rval = Tss2_Sys_PCR_SetAuthValue_Prepare(sysContext, pcrHandle, auth);
    if (rval)
        return rval;

    rval = CommonOneCall(ctx, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_PCR_SetAuthValue_Complete(sysContext);
}
