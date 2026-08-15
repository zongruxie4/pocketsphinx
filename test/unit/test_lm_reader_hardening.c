#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pocketsphinx.h>
#include <pocketsphinx/model.h>
#include <pocketsphinx/logmath.h>

#include "test_macros.h"

static char *
write_temp(const void *data, size_t len)
{
    char template[] = "/tmp/ps_lmhardXXXXXX";
    int fd;
    FILE *fp;

    fd = mkstemp(template);
    TEST_ASSERT(fd >= 0);
    fp = fdopen(fd, "wb");
    TEST_ASSERT(fp != NULL);
    if (len > 0)
        TEST_EQUAL(len, fwrite(data, 1, len, fp));
    fclose(fp);
    return strdup(template);
}

static void
put32(FILE *fp, int32 v)
{
    fwrite(&v, sizeof(v), 1, fp);
}

static void
put16(FILE *fp, uint16 v)
{
    fwrite(&v, sizeof(v), 1, fp);
}

static void
put_dmp_header(FILE *fp)
{
    put32(fp, 17);
    fwrite("Darpa Trigram LM", 1, 16, fp);
    fputc('\0', fp);
}

static char *
build_dmp(int unigrams, int bigrams, int trigrams,
          const uint16 *bigram_words, int32 weight_count,
          int truncate_word_str)
{
    char template[] = "/tmp/ps_lmhardXXXXXX";
    int fd = mkstemp(template);
    FILE *fp;
    int j;

    TEST_ASSERT(fd >= 0);
    fp = fdopen(fd, "wb");
    TEST_ASSERT(fp != NULL);

    put_dmp_header(fp);
    put32(fp, 0);              /* filename length */
    put32(fp, unigrams);       /* version > 0 means ucount, no extended header */
    put32(fp, bigrams);        /* bcount */
    put32(fp, trigrams);       /* tcount */

    /* unigram table: ucount + 1 entries, 16 bytes each */
    for (j = 0; j <= unigrams; j++) {
        put32(fp, 0);          /* mapping id */
        put32(fp, 0);          /* prob weight */
        put32(fp, 0);          /* backoff weight */
        put32(fp, 0);          /* bigram pointer */
    }

    if (bigrams > 0) {
        /* bigram table: bcount + 1 entries, 8 bytes each */
        for (j = 0; j <= bigrams; j++) {
            uint16 prob_idx = 0;
            if (bigram_words != NULL && j < bigrams)
                prob_idx = bigram_words[j];
            put16(fp, 0);              /* word id */
            put16(fp, prob_idx);       /* prob index into weight array */
            put16(fp, 0);              /* backoff index */
            put16(fp, 0);              /* bigram next */
        }
        /* prob2 weight array size */
        put32(fp, weight_count);
    }

    if (truncate_word_str) {
        /* Declare a large word-string block but supply none of it, so
         * read_word_str hits a short read and fails. */
        put32(fp, 4096);
    }

    fclose(fp);
    return strdup(template);
}

/* Defect class 1: an invalid (non-positive) DMP weight-array count in
 * read_dmp_weight_array (ngrams_raw.c) must be rejected. */
static void
test_dmp_weight_array_count(ps_config_t *config, logmath_t *lmath)
{
    ngram_model_t *lm;
    char *path = build_dmp(1, 1, 0, NULL, -1, 0);

    lm = ngram_model_read(config, path, NGRAM_AUTO, lmath);
    TEST_ASSERT(lm == NULL);
    unlink(path);
    free(path);
}

/* Defect class 1: a DMP weight-array count larger than the remaining file
 * must be rejected in read_dmp_weight_array (ngrams_raw.c) before it is
 * used to size an allocation. */
static void
test_dmp_weight_array_oversized(ps_config_t *config, logmath_t *lmath)
{
    ngram_model_t *lm;
    char *path = build_dmp(1, 1, 0, NULL, 1000000, 0);

    lm = ngram_model_read(config, path, NGRAM_AUTO, lmath);
    TEST_ASSERT(lm == NULL);
    unlink(path);
    free(path);
}

/* Defect class 5: an unchecked read_word_str failure (ngram_model_trie.c)
 * used to leave a partially built model; the reader must now fail cleanly. */
static void
test_dmp_truncated_word_str(ps_config_t *config, logmath_t *lmath)
{
    ngram_model_t *lm;
    char *path = build_dmp(1, 0, 0, NULL, 0, 1);

    lm = ngram_model_read(config, path, NGRAM_AUTO, lmath);
    TEST_ASSERT(lm == NULL);
    unlink(path);
    free(path);
}

/* Defect class 2: an ARPA file that declares a trigram section it never
 * provides makes ngrams_raw_read_arpa free a partially built array;
 * ngrams_raw_free (ngrams_raw.c) must tolerate the NULL sub-array. */
static void
test_arpa_missing_section(ps_config_t *config, logmath_t *lmath)
{
    static const char arpa[] =
        "\\data\\\n"
        "ngram 1=2\n"
        "ngram 2=1\n"
        "ngram 3=1\n"
        "\n"
        "\\1-grams:\n"
        "-1.0 <s> -0.5\n"
        "-1.0 </s> -0.5\n"
        "\n"
        "\\2-grams:\n"
        "-0.5 <s> </s> -0.3\n";
    ngram_model_t *lm;
    char *path = write_temp(arpa, sizeof(arpa) - 1);

    lm = ngram_model_read(config, path, NGRAM_AUTO, lmath);
    TEST_ASSERT(lm == NULL);
    unlink(path);
    free(path);
}

/* Defect class 3: a complete ARPA model with no end-mark used to
 * dereference a NULL line iterator in ngrams_raw_read_arpa (ngrams_raw.c);
 * the reader must now load it and only warn. */
static void
test_arpa_no_end_mark(ps_config_t *config, logmath_t *lmath)
{
    static const char arpa[] =
        "\\data\\\n"
        "ngram 1=2\n"
        "ngram 2=1\n"
        "\n"
        "\\1-grams:\n"
        "-1.0 <s> -0.5\n"
        "-1.0 </s> -0.5\n"
        "\n"
        "\\2-grams:\n"
        "-0.5 <s> </s>\n";
    ngram_model_t *lm;
    char *path = write_temp(arpa, sizeof(arpa) - 1);

    lm = ngram_model_read(config, path, NGRAM_AUTO, lmath);
    TEST_ASSERT(lm != NULL);
    ngram_model_free(lm);
    unlink(path);
    free(path);
}

int
main(int argc, char *argv[])
{
    ps_config_t *config;
    logmath_t *lmath;

    (void)argc;
    (void)argv;

    err_set_loglevel(ERR_FATAL);
    TEST_ASSERT(config = ps_config_parse_json(NULL, "{}"));
    TEST_ASSERT(lmath = logmath_init(1.0001, 0, 0));

    test_dmp_weight_array_count(config, lmath);
    test_dmp_weight_array_oversized(config, lmath);
    test_dmp_truncated_word_str(config, lmath);
    test_arpa_missing_section(config, lmath);
    test_arpa_no_end_mark(config, lmath);

    logmath_free(lmath);
    ps_config_free(config);

    return 0;
}
