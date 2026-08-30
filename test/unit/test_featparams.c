#include <pocketsphinx.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_macros.h"

static int missing_featparams_warning;

static void
log_callback(void *user_data, err_lvl_t level, const char *fmt, ...)
{
    char message[1024];
    va_list args;

    (void)user_data;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    if (level == ERR_WARN && strstr(message, "No feat.params file found"))
        missing_featparams_warning = 1;
}

int
main(int argc, char *argv[])
{
    static const char *model_files[] = {
        "mdef", "means", "mixture_weights", "noisedict",
        "transition_matrices", "variances"
    };
    char tempdir[] = "/tmp/test_featparams.XXXXXX";
    char source[1024], destination[1024];
    ps_config_t *config;
    ps_decoder_t *ps;
    size_t i;

    (void)argc;
    (void)argv;
    err_set_callback(log_callback, NULL);

    TEST_ASSERT(config = ps_config_parse_json(
                    NULL, "hmm: \"" DATADIR "/an4_ci_cont\","
                    "lm: \"" DATADIR "/turtle.lm.bin\","
                    "dict: \"" DATADIR "/turtle.dic\""));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(ps_config_str(ps_get_config(ps), "featparams") != NULL);
    TEST_ASSERT(!missing_featparams_warning);
    ps_free(ps);
    ps_config_free(config);

    TEST_ASSERT(mkdtemp(tempdir) != NULL);
    for (i = 0; i < sizeof(model_files) / sizeof(model_files[0]); ++i) {
        snprintf(source, sizeof(source), DATADIR "/an4_ci_cont/%s",
                 model_files[i]);
        snprintf(destination, sizeof(destination), "%s/%s", tempdir,
                 model_files[i]);
        TEST_EQUAL(0, symlink(source, destination));
    }

    missing_featparams_warning = 0;
    TEST_ASSERT(config = ps_config_parse_json(
                    NULL, "lm: \"" DATADIR "/turtle.lm.bin\","
                    "dict: \"" DATADIR "/turtle.dic\""));
    ps_config_set_str(config, "hmm", tempdir);
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(ps_config_str(ps_get_config(ps), "featparams") == NULL);
    TEST_ASSERT(missing_featparams_warning);
    ps_free(ps);
    ps_config_free(config);

    for (i = 0; i < sizeof(model_files) / sizeof(model_files[0]); ++i) {
        snprintf(destination, sizeof(destination), "%s/%s", tempdir,
                 model_files[i]);
        TEST_EQUAL(0, unlink(destination));
    }
    TEST_EQUAL(0, rmdir(tempdir));
    err_set_callback(err_logfp_cb, NULL);
    return 0;
}
