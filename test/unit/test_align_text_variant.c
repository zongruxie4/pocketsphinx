#include <pocketsphinx.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    ps_decoder_t *ps;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\""));
    TEST_ASSERT(ps = ps_init(config));

    TEST_EQUAL(-1, ps_set_align_text(ps, "a(99)"));
    TEST_EQUAL(0, ps_set_align_text(ps, "a a(2) are are(2)"));

    ps_free(ps);
    ps_config_free(config);
    return 0;
}
