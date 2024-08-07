/* SPDX-License-Identifier: BSD-2-Clause */
/*******************************************************************************
 * Copyright 2017-2018, Fraunhofer SIT sponsored by Infineon Technologies AG
 * All rights reserved.
 ******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h" // IWYU pragma: keep
#endif

#include <inttypes.h>         // for PRIx32, PRIx16, int32_t
#include <stddef.h>           // for NULL

#include "esys_int.h"         // for ESYS_CONTEXT, _ESYS_STATE_INIT, RSRC_NO...
#include "esys_iutil.h"       // for iesys_compute_session_value, check_sess...
#include "esys_types.h"       // for IESYS_RESOURCE
#include "tss2_common.h"      // for TSS2_RC, TSS2_RC_SUCCESS, TSS2_BASE_RC_...
#include "tss2_esys.h"        // for ESYS_CONTEXT, ESYS_TR, Esys_PCR_SetAuth...
#include "tss2_sys.h"         // for Tss2_Sys_ExecuteAsync, TSS2L_SYS_AUTH_C...
#include "tss2_tpm2_types.h"  // for TPM2B_DIGEST, TPMI_ALG_HASH, TPMI_DH_PCR

#define LOGMODULE esys
#include "util/log.h"         // for return_state_if_error, LOG_DEBUG, LOG_E...

/** One-Call function for TPM2_PCR_SetAuthPolicy
 *
 * This function invokes the TPM2_PCR_SetAuthPolicy command in a one-call
 * variant. This means the function will block until the TPM response is
 * available. All input parameters are const. The memory for non-simple output
 * parameters is allocated by the function implementation.
 *
 * @param[in,out] esysContext The ESYS_CONTEXT.
 * @param[in]  authHandle TPM2_RH_PLATFORM+{PP}.
 * @param[in]  shandle1 Session handle for authorization of authHandle
 * @param[in]  shandle2 Second session handle.
 * @param[in]  shandle3 Third session handle.
 * @param[in]  authPolicy The desired authPolicy.
 * @param[in]  hashAlg The hash algorithm of the policy.
 * @param[in]  pcrNum The PCR for which the policy is to be set.
 * @retval TSS2_RC_SUCCESS if the function call was a success.
 * @retval TSS2_ESYS_RC_BAD_REFERENCE if the esysContext or required input
 *         pointers or required output handle references are NULL.
 * @retval TSS2_ESYS_RC_BAD_CONTEXT: if esysContext corruption is detected.
 * @retval TSS2_ESYS_RC_MEMORY: if the ESAPI cannot allocate enough memory for
 *         internal operations or return parameters.
 * @retval TSS2_ESYS_RC_BAD_SEQUENCE: if the context has an asynchronous
 *         operation already pending.
 * @retval TSS2_ESYS_RC_INSUFFICIENT_RESPONSE: if the TPM's response does not
 *          at least contain the tag, response length, and response code.
 * @retval TSS2_ESYS_RC_MALFORMED_RESPONSE: if the TPM's response is corrupted.
 * @retval TSS2_ESYS_RC_RSP_AUTH_FAILED: if the response HMAC from the TPM
           did not verify.
 * @retval TSS2_ESYS_RC_MULTIPLE_DECRYPT_SESSIONS: if more than one session has
 *         the 'decrypt' attribute bit set.
 * @retval TSS2_ESYS_RC_MULTIPLE_ENCRYPT_SESSIONS: if more than one session has
 *         the 'encrypt' attribute bit set.
 * @retval TSS2_ESYS_RC_BAD_TR: if any of the ESYS_TR objects are unknown
 *         to the ESYS_CONTEXT or are of the wrong type or if required
 *         ESYS_TR objects are ESYS_TR_NONE.
 * @retval TSS2_ESYS_RC_NO_ENCRYPT_PARAM: if one of the sessions has the
 *         'encrypt' attribute set and the command does not support encryption
 *          of the first response parameter.
 * @retval TSS2_RCs produced by lower layers of the software stack may be
 *         returned to the caller unaltered unless handled internally.
 */
TSS2_RC
Esys_PCR_SetAuthPolicy(
    ESYS_CONTEXT *esysContext,
    ESYS_TR authHandle,
    ESYS_TR shandle1,
    ESYS_TR shandle2,
    ESYS_TR shandle3,
    const TPM2B_DIGEST *authPolicy,
    TPMI_ALG_HASH hashAlg,
    TPMI_DH_PCR pcrNum)
{
    TSS2_RC r;

    r = Esys_PCR_SetAuthPolicy_Async(esysContext, authHandle, shandle1, shandle2,
                                     shandle3, authPolicy, hashAlg, pcrNum);
    return_if_error(r, "Error in async function");

    /* Set the timeout to indefinite for now, since we want _Finish to block */
    int32_t timeouttmp = esysContext->timeout;
    esysContext->timeout = -1;
    /*
     * Now we call the finish function, until return code is not equal to
     * from TSS2_BASE_RC_TRY_AGAIN.
     * Note that the finish function may return TSS2_RC_TRY_AGAIN, even if we
     * have set the timeout to -1. This occurs for example if the TPM requests
     * a retransmission of the command via TPM2_RC_YIELDED.
     */
    do {
        r = Esys_PCR_SetAuthPolicy_Finish(esysContext);
        /* This is just debug information about the reattempt to finish the
           command */
        if (base_rc(r) == TSS2_BASE_RC_TRY_AGAIN)
            LOG_DEBUG("A layer below returned TRY_AGAIN: %" PRIx32
                      " => resubmitting command", r);
    } while (base_rc(r) == TSS2_BASE_RC_TRY_AGAIN);

    /* Restore the timeout value to the original value */
    esysContext->timeout = timeouttmp;
    return_if_error(r, "Esys Finish");

    return TSS2_RC_SUCCESS;
}

/** Asynchronous function for TPM2_PCR_SetAuthPolicy
 *
 * This function invokes the TPM2_PCR_SetAuthPolicy command in a asynchronous
 * variant. This means the function will return as soon as the command has been
 * sent downwards the stack to the TPM. All input parameters are const.
 * In order to retrieve the TPM's response call Esys_PCR_SetAuthPolicy_Finish.
 *
 * @param[in,out] esysContext The ESYS_CONTEXT.
 * @param[in]  authHandle TPM2_RH_PLATFORM+{PP}.
 * @param[in]  shandle1 Session handle for authorization of authHandle
 * @param[in]  shandle2 Second session handle.
 * @param[in]  shandle3 Third session handle.
 * @param[in]  authPolicy The desired authPolicy.
 * @param[in]  hashAlg The hash algorithm of the policy.
 * @param[in]  pcrNum The PCR for which the policy is to be set.
 * @retval ESYS_RC_SUCCESS if the function call was a success.
 * @retval TSS2_ESYS_RC_BAD_REFERENCE if the esysContext or required input
 *         pointers or required output handle references are NULL.
 * @retval TSS2_ESYS_RC_BAD_CONTEXT: if esysContext corruption is detected.
 * @retval TSS2_ESYS_RC_MEMORY: if the ESAPI cannot allocate enough memory for
 *         internal operations or return parameters.
 * @retval TSS2_RCs produced by lower layers of the software stack may be
           returned to the caller unaltered unless handled internally.
 * @retval TSS2_ESYS_RC_MULTIPLE_DECRYPT_SESSIONS: if more than one session has
 *         the 'decrypt' attribute bit set.
 * @retval TSS2_ESYS_RC_MULTIPLE_ENCRYPT_SESSIONS: if more than one session has
 *         the 'encrypt' attribute bit set.
 * @retval TSS2_ESYS_RC_BAD_TR: if any of the ESYS_TR objects are unknown
 *         to the ESYS_CONTEXT or are of the wrong type or if required
 *         ESYS_TR objects are ESYS_TR_NONE.
 * @retval TSS2_ESYS_RC_NO_ENCRYPT_PARAM: if one of the sessions has the
 *         'encrypt' attribute set and the command does not support encryption
 *          of the first response parameter.
 */
TSS2_RC
Esys_PCR_SetAuthPolicy_Async(
    ESYS_CONTEXT *esysContext,
    ESYS_TR authHandle,
    ESYS_TR shandle1,
    ESYS_TR shandle2,
    ESYS_TR shandle3,
    const TPM2B_DIGEST *authPolicy,
    TPMI_ALG_HASH hashAlg,
    TPMI_DH_PCR pcrNum)
{
    TSS2_RC r;
    LOG_TRACE("context=%p, authHandle=%"PRIx32 ", authPolicy=%p,"
              "hashAlg=%04"PRIx16", pcrNum=%"PRIx32 "",
              esysContext, authHandle, authPolicy, hashAlg, pcrNum);
    TSS2L_SYS_AUTH_COMMAND auths;
    RSRC_NODE_T *authHandleNode;

    /* Check context, sequence correctness and set state to error for now */
    if (esysContext == NULL) {
        LOG_ERROR("esyscontext is NULL.");
        return TSS2_ESYS_RC_BAD_REFERENCE;
    }
    r = iesys_check_sequence_async(esysContext);
    if (r != TSS2_RC_SUCCESS)
        return r;
    esysContext->state = ESYS_STATE_INTERNALERROR;

    /* Check input parameters */
    r = check_session_feasibility(shandle1, shandle2, shandle3, 1);
    return_state_if_error(r, ESYS_STATE_INIT, "Check session usage");

    /* Retrieve the metadata objects for provided handles */
    r = esys_GetResourceObject(esysContext, authHandle, &authHandleNode);
    return_state_if_error(r, ESYS_STATE_INIT, "authHandle unknown.");

    /* Initial invocation of SAPI to prepare the command buffer with parameters */
    r = Tss2_Sys_PCR_SetAuthPolicy_Prepare(esysContext->sys,
                                           (authHandleNode == NULL)
                                            ? TPM2_RH_NULL
                                            : authHandleNode->rsrc.handle,
                                           authPolicy, hashAlg, pcrNum);
    return_state_if_error(r, ESYS_STATE_INIT, "SAPI Prepare returned error.");

    /* Calculate the cpHash Values */
    r = init_session_tab(esysContext, shandle1, shandle2, shandle3);
    return_state_if_error(r, ESYS_STATE_INIT, "Initialize session resources");
    if (authHandleNode != NULL)
        iesys_compute_session_value(esysContext->session_tab[0],
                &authHandleNode->rsrc.name, &authHandleNode->auth);
    else
        iesys_compute_session_value(esysContext->session_tab[0], NULL, NULL);

    iesys_compute_session_value(esysContext->session_tab[1], NULL, NULL);
    iesys_compute_session_value(esysContext->session_tab[2], NULL, NULL);

    /* Generate the auth values and set them in the SAPI command buffer */
    r = iesys_gen_auths(esysContext, authHandleNode, NULL, NULL, &auths);
    return_state_if_error(r, ESYS_STATE_INIT,
                          "Error in computation of auth values");

    esysContext->authsCount = auths.count;
    if (auths.count > 0) {
        r = Tss2_Sys_SetCmdAuths(esysContext->sys, &auths);
        return_state_if_error(r, ESYS_STATE_INIT, "SAPI error on SetCmdAuths");
    }

    /* Trigger execution and finish the async invocation */
    r = Tss2_Sys_ExecuteAsync(esysContext->sys);
    return_state_if_error(r, ESYS_STATE_INTERNALERROR,
                          "Finish (Execute Async)");

    esysContext->state = ESYS_STATE_SENT;

    return r;
}

/** Asynchronous finish function for TPM2_PCR_SetAuthPolicy
 *
 * This function returns the results of a TPM2_PCR_SetAuthPolicy command
 * invoked via Esys_PCR_SetAuthPolicy_Finish. All non-simple output parameters
 * are allocated by the function's implementation. NULL can be passed for every
 * output parameter if the value is not required.
 *
 * @param[in,out] esysContext The ESYS_CONTEXT.
 * @retval TSS2_RC_SUCCESS on success
 * @retval ESYS_RC_SUCCESS if the function call was a success.
 * @retval TSS2_ESYS_RC_BAD_REFERENCE if the esysContext or required input
 *         pointers or required output handle references are NULL.
 * @retval TSS2_ESYS_RC_BAD_CONTEXT: if esysContext corruption is detected.
 * @retval TSS2_ESYS_RC_MEMORY: if the ESAPI cannot allocate enough memory for
 *         internal operations or return parameters.
 * @retval TSS2_ESYS_RC_BAD_SEQUENCE: if the context has an asynchronous
 *         operation already pending.
 * @retval TSS2_ESYS_RC_TRY_AGAIN: if the timeout counter expires before the
 *         TPM response is received.
 * @retval TSS2_ESYS_RC_INSUFFICIENT_RESPONSE: if the TPM's response does not
 *         at least contain the tag, response length, and response code.
 * @retval TSS2_ESYS_RC_RSP_AUTH_FAILED: if the response HMAC from the TPM did
 *         not verify.
 * @retval TSS2_ESYS_RC_MALFORMED_RESPONSE: if the TPM's response is corrupted.
 * @retval TSS2_RCs produced by lower layers of the software stack may be
 *         returned to the caller unaltered unless handled internally.
 */
TSS2_RC
Esys_PCR_SetAuthPolicy_Finish(
    ESYS_CONTEXT *esysContext)
{
    TSS2_RC r;
    LOG_TRACE("context=%p",
              esysContext);

    if (esysContext == NULL) {
        LOG_ERROR("esyscontext is NULL.");
        return TSS2_ESYS_RC_BAD_REFERENCE;
    }

    /* Check for correct sequence and set sequence to irregular for now */
    if (esysContext->state != ESYS_STATE_SENT &&
        esysContext->state != ESYS_STATE_RESUBMISSION) {
        LOG_ERROR("Esys called in bad sequence.");
        return TSS2_ESYS_RC_BAD_SEQUENCE;
    }
    esysContext->state = ESYS_STATE_INTERNALERROR;

    /*Receive the TPM response and handle resubmissions if necessary. */
    r = Tss2_Sys_ExecuteFinish(esysContext->sys, esysContext->timeout);
    if (base_rc(r) == TSS2_BASE_RC_TRY_AGAIN) {
        LOG_DEBUG("A layer below returned TRY_AGAIN: %" PRIx32, r);
        esysContext->state = ESYS_STATE_SENT;
        return r;
    }
    /* This block handle the resubmission of TPM commands given a certain set of
     * TPM response codes. */
    if (r == TPM2_RC_RETRY || r == TPM2_RC_TESTING || r == TPM2_RC_YIELDED) {
        LOG_DEBUG("TPM returned RETRY, TESTING or YIELDED, which triggers a "
            "resubmission: %" PRIx32, r);
        if (esysContext->submissionCount++ >= ESYS_MAX_SUBMISSIONS) {
            LOG_WARNING("Maximum number of (re)submissions has been reached.");
            esysContext->state = ESYS_STATE_INIT;
            return r;
        }
        esysContext->state = ESYS_STATE_RESUBMISSION;
        r = Tss2_Sys_ExecuteAsync(esysContext->sys);
        if (r != TSS2_RC_SUCCESS) {
            LOG_WARNING("Error attempting to resubmit");
            /* We do not set esysContext->state here but inherit the most recent
             * state of the _async function. */
            return r;
        }
        r = TSS2_ESYS_RC_TRY_AGAIN;
        LOG_DEBUG("Resubmission initiated and returning RC_TRY_AGAIN.");
        return r;
    }
    /* The following is the "regular error" handling. */
    if (iesys_tpm_error(r)) {
        LOG_WARNING("Received TPM Error");
        esysContext->state = ESYS_STATE_INIT;
        return r;
    } else if (r != TSS2_RC_SUCCESS) {
        LOG_ERROR("Received a non-TPM Error");
        esysContext->state = ESYS_STATE_INTERNALERROR;
        return r;
    }

    /*
     * Now the verification of the response (hmac check) and if necessary the
     * parameter decryption have to be done.
     */
    r = iesys_check_response(esysContext);
    return_state_if_error(r, ESYS_STATE_INTERNALERROR,
                          "Error: check response");

    /*
     * After the verification of the response we call the complete function
     * to deliver the result.
     */
    r = Tss2_Sys_PCR_SetAuthPolicy_Complete(esysContext->sys);
    return_state_if_error(r, ESYS_STATE_INTERNALERROR,
                          "Received error from SAPI unmarshaling" );

    esysContext->state = ESYS_STATE_INIT;

    return TSS2_RC_SUCCESS;
}
