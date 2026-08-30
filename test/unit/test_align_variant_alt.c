#include <pocketsphinx.h>

#include "lm/fsg_model.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    fsg_model_t *fsg;
    ps_config_t *config;
    ps_decoder_t *ps;
    int32 altwid;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\""));
    TEST_ASSERT(ps = ps_init(config));

    TEST_EQUAL(0, ps_set_align_text(ps, "a"));
    TEST_ASSERT(fsg = ps_get_fsg(ps, "_align"));
    TEST_ASSERT(fsg_model_has_alt(fsg));
    TEST_ASSERT((altwid = fsg_model_word_id(fsg, "a(2)")) >= 0);
    TEST_ASSERT(fsg_model_is_alt(fsg, altwid));

    TEST_EQUAL(0, ps_set_align_text(ps, "a(2)"));
    TEST_ASSERT(fsg = ps_get_fsg(ps, "_align"));
    TEST_ASSERT(fsg_model_word_id(fsg, "a(2)") >= 0);
    TEST_ASSERT(!fsg_model_has_alt(fsg));

    ps_free(ps);
    ps_config_free(config);
    return 0;
}
