/* SPDX-License-Identifier: BSD-2-Clause */

#ifdef HAVE_CONFIG_H
#include "config.h" // IWYU pragma: keep
#endif

#include <errno.h>          // for errno, EINTR
#include <inttypes.h>       // for uint8_t, PRIu32
#include <stdbool.h>        // for true, bool, false
#include <stdio.h>          // for ferror, fread, setvbuf, stdin, size_t
#include <stdlib.h>         // for exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>         // for strcmp, strerror, memcmp
#include <unistd.h>         // for getpid

#include "tcti-cmd-test.h"  // for getcap_command, getcap_good_resp
#include "tcti-common.h"    // for tpm_header_t, TPM_HEADER_SIZE, header_unm...
#include "tss2_common.h"    // for TSS2_RC, TSS2_RC_SUCCESS

#define LOGMODULE test
#include "util/log.h"       // for LOG_ERROR, LOG_DEBUG, LOGBLOB_DEBUG

/*
 * A malformed header response. The header reports smaller than the actual
 * payload.
 */
static uint8_t getcap_hdr_malformed_size_smaller[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x01, 0x00, 0x32,
        0x2e, 0x30, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x01, 0x03, 0x00,
        0x00, 0x00, 0xa7, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x07, 0xe1, 0x00,
        0x00, 0x01, 0x05, 0x49, 0x42, 0x4d, 0x20, 0x00, 0x00, 0x01, 0x06, 0x53,
        0x57, 0x20, 0x20, 0x00, 0x00, 0x01, 0x07, 0x20, 0x54, 0x50, 0x4d, 0x00,
        0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x01, 0x0b, 0x20, 0x16, 0x05, 0x11, 0x00, 0x00, 0x01, 0x0c, 0x00,
        0x16, 0x28, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x00, 0x00, 0x04, 0x00, 0x00,
        0x00, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, 0x0f, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x12, 0x00,
        0x00, 0x00, 0x18, 0x00, 0x00, 0x01, 0x13, 0x00, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x01, 0x14, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x01, 0x16, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x17, 0x00, 0x00, 0x08, 0x00, 0x00,
        0x00, 0x01, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x19, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x1a, 0x00, 0x00, 0x00, 0x0c, 0x00,
        0x00, 0x01, 0x1b, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x1c, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1d, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x01, 0x1e, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x1f, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x00, 0x30, 0x00,
        0x00, 0x01, 0x21, 0x00, 0x00, 0x0c, 0xe4, 0x00, 0x00, 0x01, 0x22, 0x00,
        0x00, 0x01, 0x44, 0x00, 0x00, 0x01, 0x23, 0x32, 0x2e, 0x30, 0x00, 0x00,
        0x00, 0x01, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x25, 0x00,
        0x00, 0x00, 0x92, 0x00, 0x00, 0x01, 0x26, 0x00, 0x00, 0x00, 0xa7, 0x00,
        0x00, 0x01, 0x27, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x01, 0x28, 0x00,
        0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x29, 0x00, 0x00, 0x00, 0x71, 0x00,
        0x00, 0x01, 0x2a, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x01, 0x2b, 0x00,
        0x00, 0x00, 0x04, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x00, 0x04, 0x00, 0x00,
        0x00, 0x01, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x2e, 0x00,
        0x00, 0x04, 0x00
};
/*
 * A malformed header response. The header reports bigger than the actual
 * payload.
 */
static uint8_t getcap_resp_malformed_size_bigger[] = {
        0x80, 0x01, 0x00, 0x00, 0x42, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x01, 0x00, 0x32,
        0x2e, 0x30, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x01, 0x03, 0x00,
        0x00, 0x00, 0xa7, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x07, 0xe1, 0x00,
        0x00, 0x01, 0x05, 0x49, 0x42, 0x4d, 0x20, 0x00, 0x00, 0x01, 0x06, 0x53,
        0x57, 0x20, 0x20, 0x00, 0x00, 0x01, 0x07, 0x20, 0x54, 0x50, 0x4d, 0x00,
        0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x01, 0x0b, 0x20, 0x16, 0x05, 0x11, 0x00, 0x00, 0x01, 0x0c, 0x00,
        0x16, 0x28, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x00, 0x00, 0x04, 0x00, 0x00,
        0x00, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, 0x0f, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01, 0x12, 0x00,
        0x00, 0x00, 0x18, 0x00, 0x00, 0x01, 0x13, 0x00, 0x00, 0x00, 0x03, 0x00,
        0x00, 0x01, 0x14, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x01, 0x16, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x17, 0x00, 0x00, 0x08, 0x00, 0x00,
        0x00, 0x01, 0x18, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x19, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x1a, 0x00, 0x00, 0x00, 0x0c, 0x00,
        0x00, 0x01, 0x1b, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x01, 0x1c, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x1d, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x01, 0x1e, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x1f, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x00, 0x30, 0x00,
        0x00, 0x01, 0x21, 0x00, 0x00, 0x0c, 0xe4, 0x00, 0x00, 0x01, 0x22, 0x00,
        0x00, 0x01, 0x44, 0x00, 0x00, 0x01, 0x23, 0x32, 0x2e, 0x30, 0x00, 0x00,
        0x00, 0x01, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x25, 0x00,
        0x00, 0x00, 0x92, 0x00, 0x00, 0x01, 0x26, 0x00, 0x00, 0x00, 0xa7, 0x00,
        0x00, 0x01, 0x27, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x01, 0x28, 0x00,
        0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x29, 0x00, 0x00, 0x00, 0x71, 0x00,
        0x00, 0x01, 0x2a, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x01, 0x2b, 0x00,
        0x00, 0x00, 0x04, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x00, 0x04, 0x00, 0x00,
        0x00, 0x01, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x2e, 0x00,
        0x00, 0x04, 0x00
};
/*
 * A malformed header response. The size is smaller than a header.
 */
static uint8_t getcap_resp_malformed_short[] = {
        0x80, 0x01, 0x00, 0x00, 0x42, 0x83, 0x00, 0x00
};

#define child_exit(code) do { LOG_ERROR ("PID (%d): Child child_exiting", getpid ()); exit (code); } while (0)

int main (int argc, char *argv[])
{
    /* No buffering on read/write from child stdio/stdin */
    setvbuf (stdin, NULL, _IONBF, 0);
    setvbuf (stdout, NULL, _IONBF, 0);

    /*
     * Figure out the response the test wants by mapping strings to hard-coded
     * response buffers. We default to good if no argument is specified.
     */
    uint8_t *response_buffer = getcap_good_resp;
    size_t response_buffer_size = sizeof (getcap_good_resp);
    char *response_selector = "good";
    if (argc == 2) {
        response_selector = argv[1];
    } else if (argc > 2) {
        LOG_ERROR ("Expected only 1 argument, got: %d", argc - 1);
        child_exit (EXIT_SUCCESS);
    }

    LOG_DEBUG ("CHILD (%d): Response selector is: %s", getpid (),
            response_selector);

    if (!strcmp (response_selector, "good")) {
        /* nothing to do already set */
    } else if (!strcmp (response_selector, "small")) {
        response_buffer = getcap_hdr_malformed_size_smaller;
        response_buffer_size = sizeof (getcap_hdr_malformed_size_smaller);
    } else if (!strcmp (response_selector, "big")) {
        response_buffer = getcap_resp_malformed_size_bigger;
        response_buffer_size = sizeof (getcap_resp_malformed_size_bigger);
    } else if (!strcmp (response_selector, "short")) {
        response_buffer = getcap_resp_malformed_short;
        response_buffer_size = sizeof (getcap_resp_malformed_short);
    } else {
        LOG_ERROR ("Unknown buffer response string: %s", argv[1]);
        child_exit (EXIT_FAILURE);
    }

    /*
     * The child now:
     *   - waits for a command over stdin
     *   - checks the command against a known good value
     *   - writes the user requested response to stdout
     */
    bool quit = false;

    while (!quit) {

        uint8_t buf[4096];

        LOG_DEBUG ("PID (%d): Child waiting on TPM command", getpid ());

        size_t read_size = fread (buf, 1, TPM_HEADER_SIZE, stdin);
        if (read_size != TPM_HEADER_SIZE || ferror (stdin)) {
            if (ferror (stdin) && errno == EINTR) {
                quit = true;
                LOG_ERROR ("PID (%d): Child quiting", getpid ());
                continue;
            }
            LOG_ERROR ("Could not get TPM Header from stdin: %s",
                    strerror (errno));
            child_exit (EXIT_FAILURE);
        }

        LOGBLOB_DEBUG (buf, read_size, "PID (%d): Child got TPM command header",
                getpid ());

        tpm_header_t hdr = { 0 };
        TSS2_RC rc = header_unmarshal (buf, &hdr);
        if (rc != TSS2_RC_SUCCESS) {
            LOG_ERROR ("PID (%d): could not unmarshal header", getpid ());
            LOG_ERROR ("Could not unmarshal header");
            child_exit (EXIT_FAILURE);
        }

        if (hdr.size < TPM_HEADER_SIZE) {
            LOG_ERROR (
                    "Header size field cannot be smaller than header, " "got: %" PRIu32,
                    hdr.size);
            LOG_ERROR ("PID (%d): header size too small: %" PRIu32, getpid (),
                    hdr.size);
            child_exit (EXIT_FAILURE);
        }

        size_t data_size = hdr.size - TPM_HEADER_SIZE;

        LOG_DEBUG ("PID (%d): Child waiting on remaining tpm: %zu", getpid (),
                data_size);

        read_size = fread (&buf[TPM_HEADER_SIZE], 1, data_size, stdin);
        if (read_size != data_size || ferror (stdin)) {
            LOG_ERROR (
                    "PID (%d): Command payload %zu != %zu, full read failed: %s",
                    getpid (), read_size, data_size, strerror (errno));
            LOG_ERROR ("Command payload full read failed: %s", strerror (errno));
            child_exit (EXIT_FAILURE);
        }

        LOGBLOB_DEBUG (buf, data_size, "PID (%d): Child got full TPM command",
                getpid ());

        /*
         * We know this is the *only* command we will get, so it should be
         * equal
         */
        if (sizeof (getcap_command) != hdr.size) {
            LOG_ERROR (
                    "PID (%d): sizeof (getcap_command) != hdr.size, %zu != %"PRIu32,
                    getpid (), sizeof (getcap_command), hdr.size);
            LOG_ERROR ("Unexpected command size, got %" PRIu32 ", expected %zu",
                    hdr.size, sizeof (getcap_command));
            child_exit (EXIT_FAILURE);
        }
        if (memcmp (getcap_command, buf, hdr.size)) {
            LOG_ERROR ("PID (%d): memcmp failed", getpid ());
            LOG_ERROR ("Unexpected command buffer");
            child_exit (EXIT_FAILURE);
        }

        LOGBLOB_DEBUG (response_buffer, response_buffer_size,
                "PID (%d): Child writing to stdout", getpid ());

        size_t bytes_wrote = fwrite (response_buffer, 1, response_buffer_size,
        stdout);

        if (bytes_wrote != response_buffer_size || ferror (stdout)) {
            if (ferror (stdout)) {
                if (errno == EINTR) {
                    quit = true;
                    continue;
                }
                LOG_ERROR ("Could not write response buffer: %s",
                        strerror (errno));
            } else {
                LOG_ERROR ("bytes_wrote (%zu) != response_buffer_size (%zu)",
                        bytes_wrote, response_buffer_size);
            }
            child_exit (EXIT_FAILURE);
        }

        LOG_DEBUG ("PID (%d): Child wrote to stdout", getpid ());
    }

    child_exit (EXIT_SUCCESS);
}
