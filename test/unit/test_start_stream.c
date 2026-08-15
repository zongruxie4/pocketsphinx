#include <pocketsphinx.h>

#include "pocketsphinx_internal.h"
#include "fe/fe_internal.h"
#include "fe/fe_noise.h"
#include "test_macros.h"

static ps_config_t *
make_config(void)
{
    return ps_config_parse_json(
        NULL,
        "hmm: \"" DATADIR "/an4_ci_cont\","
        "lm: \"" DATADIR "/turtle.lm.bin\","
        "dict: \"" DATADIR "/turtle.dic\","
        "samprate: 16000");
}

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    ps_decoder_t *ps;

    (void)argc;
    (void)argv;

    TEST_ASSERT(config = make_config());
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(!ps_config_bool(ps_get_config(ps), "remove_noise"));
    TEST_EQUAL(0, ps_start_stream(ps));
    ps_free(ps);
    ps_config_free(config);

    TEST_ASSERT(config = make_config());
    TEST_ASSERT(ps_config_set_bool(config, "remove_noise", TRUE));
    TEST_ASSERT(ps = ps_init(config));
    TEST_ASSERT(ps_config_bool(ps_get_config(ps), "remove_noise"));
    TEST_ASSERT(ps->acmod->fe->noise_stats != NULL);
    ps->acmod->fe->noise_stats->undefined = FALSE;
    TEST_EQUAL(0, ps_start_stream(ps));
    TEST_ASSERT(ps->acmod->fe->noise_stats->undefined);
    ps_free(ps);
    ps_config_free(config);

    return 0;
}
