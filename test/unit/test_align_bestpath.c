/* -*- c-basic-offset: 4 -*- */
#include <stdarg.h>
#include <string.h>

#include <pocketsphinx.h>

#include "test_macros.h"

#define ALIGN_TEXT "feels like these days go on forever"

static void
capture_error(void *user_data, err_lvl_t level, const char *fmt, ...)
{
    char message[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (strstr(message, "impossible duration") != NULL)
        *(int *)user_data = TRUE;
    (void)level;
}

static int
decode_wav(ps_decoder_t *ps)
{
    FILE *rawfh;
    long nsamp;

    TEST_ASSERT(rawfh = fopen(DATADIR "/forever/input_2_16k.wav", "rb"));
    TEST_EQUAL(0, fseek(rawfh, 44, SEEK_SET));
    nsamp = ps_decode_raw(ps, rawfh, -1);
    fclose(rawfh);
    TEST_ASSERT(nsamp > 0);
    return 0;
}

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    ps_decoder_t *ps;
    int impossible_duration = FALSE;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "lm: \"" MODELDIR "/en-us/en-us.lm.bin\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\""));
    TEST_ASSERT(ps_config_bool(config, "bestpath"));
    TEST_ASSERT(ps = ps_init(config));
    err_set_callback(capture_error, &impossible_duration);

    TEST_EQUAL(0, ps_set_align_text(ps, ALIGN_TEXT));
    decode_wav(ps);
    TEST_EQUAL(0, ps_set_alignment(ps, NULL));
    decode_wav(ps);

    err_set_callback(err_logfp_cb, NULL);
    TEST_ASSERT(impossible_duration == FALSE);
    ps_free(ps);
    ps_config_free(config);
    return 0;
}
